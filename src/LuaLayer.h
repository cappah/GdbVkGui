#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct lua_State lua_State;

    lua_State* InitLuaState(void);

    lua_State* GetLuaState(void);

    int ParseLuaFile(const char* filename);

    void ReloadModule(const char* module_name);

    int EnterLuaCallback(int global_ref, int func_ref);

    void ExitLuaCallback(void);

    typedef struct Vec4
    {
        union
        {
            struct
            {
                float x, y, z, w;
            };
            float raw[4];
        };
    } Vec4;

    void PushFBufferToLua(float fbuff[], int fbuff_sz);

    void ReadFBufferFromLua(float fbuff[], int fbuff_sz, int index);

#ifdef __cplusplus
}
#endif
