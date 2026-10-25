#pragma once
#include <cstring>

namespace Hooking
{
    inline void* __nullptrForHook = nullptr;

    bool InternalHook(uintptr_t Ptr, void* Detour, void** Original);

    static inline void* FnToVoid(const void* p)
    {
        void* out = nullptr;
        memcpy(&out, &p, sizeof(out) < sizeof(p) ? sizeof(out) : sizeof(p));
        return out;
    }

    template <class PtrType = void*, class OriginalType = void*, class DetourType = void*>
    std::enable_if_t<!std::is_base_of_v<UObject, OriginalType>, void> Hook(PtrType _Ptr, DetourType _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        void* detourVoid = nullptr;
        memcpy(&detourVoid, &_Detour, sizeof(detourVoid) < sizeof(_Detour) ? sizeof(detourVoid) : sizeof(_Detour));
        InternalHook((uintptr_t)_Ptr, detourVoid, (LPVOID*)(std::is_same_v<OriginalType, void*> ? nullptr : (void*)&_Orig));
    }

    bool EnableHook(uintptr_t Ptr);
    bool DisableHook(uintptr_t Ptr);

    inline void VirtualSwap(void** VTable, int Index, void* Detour, void** Original = nullptr);

    template <typename DetourType>
    inline void VirtualSwapFn(void** VTable, int Index, DetourType Detour, void** Original = nullptr)
    {
        void* d = nullptr;
        memcpy(&d, &Detour, sizeof(d) < sizeof(Detour) ? sizeof(d) : sizeof(Detour));
        VirtualSwap(VTable, Index, d, Original);
    }

    template <typename ClassType, typename OriginalType = void*, typename DetourType = void*>
    __forceinline void Hook(uint32_t _Ind, DetourType _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        auto _Vt = ClassType::GetDefaultObj()->Vft;

        if constexpr (!std::is_same_v<OriginalType, void*>)
            _Orig = (OriginalType)(void*)_Vt[_Ind];

        DWORD _Vo;
        VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
        void* detourVoid = nullptr;
        memcpy(&detourVoid, &_Detour, sizeof(detourVoid) < sizeof(_Detour) ? sizeof(detourVoid) : sizeof(_Detour));
        _Vt[_Ind] = detourVoid;
        VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
    }

    template <typename ClassType, typename DetourType = void*>
    __forceinline static void HookEvery(uint32_t _Ind, DetourType _Detour)
    {
        for (int32_t i = 0; i < TUObjectArray::Num(); i++)
        {
            auto Obj = TUObjectArray::GetObjectByIndex(i);

            if (Obj && Obj->IsDefaultObject() && Obj->IsA<ClassType>())
            {
                auto ObjectVTable = Obj->Vft;

                DWORD og;
                VirtualProtect(ObjectVTable + _Ind, 8, PAGE_EXECUTE_READWRITE, &og);
                void* detourVoid = nullptr;
                memcpy(&detourVoid, &_Detour, sizeof(detourVoid) < sizeof(_Detour) ? sizeof(detourVoid) : sizeof(_Detour));
                ObjectVTable[_Ind] = detourVoid;
                VirtualProtect(ObjectVTable + _Ind, 8, og, &og);
            }
        }
    }

    template <typename OriginalType = void*, typename DetourType = void*>
    __forceinline void ExecHook(UFunction* _Fn, DetourType _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        if (!_Fn)
            return;
        if constexpr (!std::is_same_v<OriginalType, void*>)
        {
            void* rawOrig = nullptr;
            void* fnOrig = nullptr;
            memcpy(&fnOrig, &_Fn->ExecFunction, sizeof(fnOrig));
            memcpy(&rawOrig, &fnOrig, sizeof(rawOrig));
            _Orig = (OriginalType)(rawOrig);
        }

        void* rawDetour = nullptr;
        memcpy(&rawDetour, &_Detour, sizeof(rawDetour) < sizeof(_Detour) ? sizeof(rawDetour) : sizeof(_Detour));
        _Fn->ExecFunction = rawDetour; // MSVC property (void*)
    }

    template <typename PatchType>
    __forceinline void Patch(uintptr_t ptr, PatchType byte)
    {
        DWORD og;
        VirtualProtect(LPVOID(ptr), sizeof(PatchType), PAGE_EXECUTE_READWRITE, &og);
        *(PatchType*)ptr = byte;
        VirtualProtect(LPVOID(ptr), sizeof(PatchType), og, &og);
    }

    void Null(uintptr_t Ptr);
    void RetTrue(uintptr_t Ptr);

#ifdef __ANDROID__
    // Android ARM64 extras (implemented in android/erbium/HookingARM64.cpp)
    void Nop(uintptr_t Ptr, uint32_t count = 1);
    bool InvertBranch(uintptr_t Ptr);
#endif

    uint8_t* AllocateShellcodeRegion(void* targetAddr, size_t SizeOfShellcode);

    void InternalRel32Hook(uintptr_t _Ptr, void* _Detour, void** _Orig);

    template <class PtrType = void*, class OriginalType = void*, class DetourType = void*>
    void Rel32Hook(PtrType _Ptr, DetourType _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        void* detourVoid = nullptr;
        memcpy(&detourVoid, &_Detour, sizeof(detourVoid) < sizeof(_Detour) ? sizeof(detourVoid) : sizeof(_Detour));
        InternalRel32Hook((uintptr_t)_Ptr, detourVoid, (LPVOID*)(std::is_same_v<OriginalType, void*> ? nullptr : (void*)&_Orig));
    }
} // namespace Hooking