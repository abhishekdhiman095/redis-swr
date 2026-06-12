#include "redismodule.h"
#include <string.h>

/*
 * SETSWR command
 * Placeholder implementation
 */
int SetSWRCommand(
    RedisModuleCtx *ctx,
    RedisModuleString **argv,
    int argc)
{
    // Expected:
    // SETSWR key value SOFT seconds HARD seconds

    if (argc != 7 && argc != 9)
    {
        RedisModule_WrongArity(ctx);
        return REDISMODULE_OK;
    }


    // argv[1] = key
    // argv[2] = value
    // argv[3] = SOFT
    // argv[4] = soft seconds
    // argv[5] = HARD
    // argv[6] = hard seconds


    long long softTTL;
    long long hardTTL;


    if (RedisModule_StringToLongLong(
            argv[4],
            &softTTL) != REDISMODULE_OK)
    {
        RedisModule_ReplyWithError(
            ctx,
            "Invalid soft expiry"
        );

        return REDISMODULE_OK;
    }


    if (RedisModule_StringToLongLong(
            argv[6],
            &hardTTL) != REDISMODULE_OK)
    {
        RedisModule_ReplyWithError(
            ctx,
            "Invalid hard expiry"
        );

        return REDISMODULE_OK;
    }


    if (softTTL >= hardTTL)
    {
        RedisModule_ReplyWithError(
            ctx,
            "Soft expiry must be less than hard expiry"
        );

        return REDISMODULE_OK;
    }


    // Current Unix timestamp
    long long now = RedisModule_Milliseconds() / 1000;


    long long softExpiry = now + softTTL;
    long long hardExpiry = now + hardTTL;



    /*
       Create internal key

       Example:

       user:123

       becomes:

       swr:user:123
    */

    RedisModuleString *swrKey =
        RedisModule_CreateStringPrintf(
            ctx,
            "swr:%s",
           RedisModule_StringPtrLen(argv[1], NULL)
        );


    RedisModuleKey *key =
        RedisModule_OpenKey(
            ctx,
            swrKey,
            REDISMODULE_WRITE
        );


    /*
        Store:

        value
        softExpiry
        hardExpiry
    */


   RedisModuleString *softExpiryStr =
    RedisModule_CreateStringFromLongLong(
        ctx,
        softExpiry
    );

RedisModuleString *hardExpiryStr =
    RedisModule_CreateStringFromLongLong(
        ctx,
        hardExpiry
    );

RedisModuleString *refreshChannel = NULL;

if (argc == 9)
{
    if (strcasecmp(
            RedisModule_StringPtrLen(argv[7], NULL),
            "CHANNEL") != 0)
    {
        RedisModule_ReplyWithError(
            ctx,
            "Expected CHANNEL"
        );
        return REDISMODULE_OK;
    }

    refreshChannel = argv[8];
}

if (refreshChannel != NULL)
    {
        RedisModule_HashSet(
            key,
            REDISMODULE_HASH_NONE,
            RedisModule_CreateString(ctx, "value", 5),
            argv[2],
            RedisModule_CreateString(ctx, "softExpiry", 10),
            softExpiryStr,
            RedisModule_CreateString(ctx, "hardExpiry", 10),
            hardExpiryStr,
            RedisModule_CreateString(ctx, "refreshChannel", 14),
            refreshChannel,
            NULL
        );
    }else{
        RedisModule_HashSet(
            key,
            REDISMODULE_HASH_NONE,

            RedisModule_CreateString(ctx, "value", 5),
            argv[2],

            RedisModule_CreateString(ctx, "softExpiry", 10),
            softExpiryStr,

            RedisModule_CreateString(ctx, "hardExpiry", 10),
            hardExpiryStr,
            NULL
        );
    }


    RedisModule_CloseKey(key);


    RedisModule_FreeString(
        ctx,
        swrKey
    );


    RedisModule_ReplyWithSimpleString(
        ctx,
        "OK"
    );


    return REDISMODULE_OK;
}


/*
 * GETSWR command
 * Placeholder implementation
 */
int GetSWRCommand(
    RedisModuleCtx *ctx,
    RedisModuleString **argv,
    int argc)
{
    if (argc != 2)
    {
        RedisModule_WrongArity(ctx);
        return REDISMODULE_OK;
    }


    /*
        Internal key:

        product:123

        becomes:

        swr:product:123
    */

    RedisModuleString *swrKey =
        RedisModule_CreateStringPrintf(
            ctx,
            "swr:%s",
            RedisModule_StringPtrLen(argv[1], NULL)
        );


    RedisModuleKey *key =
        RedisModule_OpenKey(
            ctx,
            swrKey,
            REDISMODULE_READ
        );


    /*
       Check if key exists
    */

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY)
    {
        RedisModule_CloseKey(key);
        RedisModule_FreeString(ctx, swrKey);

        RedisModule_ReplyWithNull(ctx);

        return REDISMODULE_OK;
    }


    /*
       Read fields
    */

RedisModuleString *value = NULL;
RedisModuleString *softExpiryStr = NULL;
RedisModuleString *hardExpiryStr = NULL;
RedisModuleString *refreshChannel = NULL;

RedisModuleString *valueField =
    RedisModule_CreateString(ctx, "value", 5);

RedisModuleString *softField =
    RedisModule_CreateString(ctx, "softExpiry", 10);

RedisModuleString *hardField =
    RedisModule_CreateString(ctx, "hardExpiry", 10);


RedisModule_HashGet(
    key,
    REDISMODULE_HASH_CFIELDS,

    "value",
    &value,

    "softExpiry",
    &softExpiryStr,

    "hardExpiry",
    &hardExpiryStr,

    "refreshChannel",
    &refreshChannel,

    NULL
);


RedisModule_FreeString(ctx, valueField);
RedisModule_FreeString(ctx, softField);
RedisModule_FreeString(ctx, hardField);


    long long softExpiry;
    long long hardExpiry;


    RedisModule_StringToLongLong(
        softExpiryStr,
        &softExpiry
    );


    RedisModule_StringToLongLong(
        hardExpiryStr,
        &hardExpiry
    );


    long long now =
        RedisModule_Milliseconds() / 1000;



    /*
       Determine state
    */

    if (now >= hardExpiry)
    {
        /*
          Hard expiry reached

          Delete key
        */

        RedisModule_CloseKey(key);

        RedisModuleKey *writeKey =
            RedisModule_OpenKey(
                ctx,
                swrKey,
                REDISMODULE_WRITE
            );

        RedisModule_DeleteKey(writeKey);

        RedisModule_CloseKey(writeKey);


        RedisModule_FreeString(ctx, swrKey);


        RedisModule_ReplyWithNull(ctx);

        return REDISMODULE_OK;
    }


    if (now < softExpiry)
    {
        /*
           Fresh data
        */

        RedisModule_ReplyWithArray(ctx, 2);

        RedisModule_ReplyWithSimpleString(
            ctx,
            "FRESH"
        );

        RedisModule_ReplyWithString(
            ctx,
            value
        );
    }
    else
    {
        /*
           Stale data
        */

        RedisModule_ReplyWithArray(ctx, 2);

        RedisModule_ReplyWithSimpleString(
            ctx,
            "STALE"
        );

        RedisModule_ReplyWithString(
            ctx,
            value
        );

    /*
     * Trigger refresh only if channel exists
     */
        // if (refreshChannel != NULL)
        // {
        //     RedisModule_PublishMessage(
        //         ctx,
        //         refreshChannel,
        //         argv[1]
        //     );
        // }

        if (refreshChannel != NULL)
{
    /*
     * Lock key:
     * swr:lock:<original-key>
     */

    RedisModuleString *lockKey =
        RedisModule_CreateStringPrintf(
            ctx,
            "swr:lock:%s",
            RedisModule_StringPtrLen(
                argv[1],
                NULL
            )
        );

    RedisModuleString *lockValue =
        RedisModule_CreateString(
            ctx,
            "1",
            1
        );

    RedisModuleString *nx =
        RedisModule_CreateString(
            ctx,
            "NX",
            2
        );

    RedisModuleString *px =
        RedisModule_CreateString(
            ctx,
            "PX",
            2
        );

    RedisModuleString *ttl =
        RedisModule_CreateStringFromLongLong(
            ctx,
            30000   /* 30 seconds */
        );

    /*
     * Equivalent Redis command:
     *
     * SET swr:lock:key 1 NX PX 30000
     */

    RedisModuleCallReply *reply =
        RedisModule_Call(
            ctx,
            "SET",
            "sssss",
            lockKey,
            lockValue,
            nx,
            px,
            ttl
        );

    /*
     * If SET returned OK,
     * lock acquired.
     */

    if (reply != NULL &&
        RedisModule_CallReplyType(reply) ==
        REDISMODULE_REPLY_STRING)
    {
        int subscribers =
            RedisModule_PublishMessage(
                ctx,
                refreshChannel,
                argv[1]
            );

        RedisModule_Log(
            ctx,
            "notice",
            "Published refresh event to %d subscribers",
            subscribers
        );
    }

    RedisModule_FreeCallReply(reply);

    RedisModule_FreeString(ctx, lockKey);
    RedisModule_FreeString(ctx, lockValue);
    RedisModule_FreeString(ctx, nx);
    RedisModule_FreeString(ctx, px);
    RedisModule_FreeString(ctx, ttl);
}
    }


    RedisModule_CloseKey(key);

    RedisModule_FreeString(
        ctx,
        swrKey
    );


    return REDISMODULE_OK;
}


/*
 * Redis module entry point
 */
int RedisModule_OnLoad(
    RedisModuleCtx *ctx,
    RedisModuleString **argv,
    int argc)
{
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);


    if (RedisModule_Init(
            ctx,
            "swr",
            1,
            REDISMODULE_APIVER_1
        ) == REDISMODULE_ERR)
    {
        return REDISMODULE_ERR;
    }


    if (RedisModule_CreateCommand(
            ctx,
            "setswr",
            SetSWRCommand,
            "write",
            1,
            1,
            1
        ) == REDISMODULE_ERR)
    {
        return REDISMODULE_ERR;
    }


    if (RedisModule_CreateCommand(
            ctx,
            "getswr",
            GetSWRCommand,
            "readonly",
            1,
            1,
            1
        ) == REDISMODULE_ERR)
    {
        return REDISMODULE_ERR;
    }


    return REDISMODULE_OK;
}