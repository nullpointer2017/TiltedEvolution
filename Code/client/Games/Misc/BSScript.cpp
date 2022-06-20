#include <Misc/BSScript.h>
#include <Misc/NativeFunction.h>

#include <World.h>
#include <Events/PapyrusFunctionRegisterEvent.h>
#include <Forms/TESForm.h>
#include <Misc/GameVM.h>
#include <Actor.h>
#include <PlayerCharacter.h>
#include <Games/ActorExtension.h>

TP_THIS_FUNCTION(TRegisterPapyrusFunction, void, BSScript::IVirtualMachine, NativeFunction*);
TP_THIS_FUNCTION(TCompareVariables, int64_t, void, BSScript::Variable*, BSScript::Variable*);

TRegisterPapyrusFunction* RealRegisterPapyrusFunction = nullptr;
TCompareVariables* RealCompareVariables = nullptr;

void TP_MAKE_THISCALL(HookRegisterPapyrusFunction, BSScript::IVirtualMachine, NativeFunction* apFunction)
{
    auto& runner = World::Get().GetRunner();

    PapyrusFunctionRegisterEvent event(apFunction->functionName.AsAscii(), apFunction->typeName.AsAscii(), apFunction->functionAddress);

    runner.Trigger(std::move(event));

    ThisCall(RealRegisterPapyrusFunction, apThis, apFunction);
}

// TODO: ft
int64_t TP_MAKE_THISCALL(HookCompareVariables, void, BSScript::Variable* apVar1, BSScript::Variable* apVar2)
{
#if TP_SKYRIM64
    BSScript::Object* pObject1 = apVar1->GetObject();
    BSScript::Object* pObject2 = apVar2->GetObject();

    if (!pObject1 || !pObject2)
        return ThisCall(RealCompareVariables, apThis, apVar1, apVar2);

    uint64_t handle1 = pObject1->GetHandle();
    uint64_t handle2 = pObject2->GetHandle();

    auto* pPolicy = GameVM::Get()->virtualMachine->GetObjectHandlePolicy();

    if (!pPolicy ||
        !handle1 ||
        !handle2 ||
        !pPolicy->HandleIsType((uint32_t)Actor::Type, handle1) ||
        !pPolicy->HandleIsType((uint32_t)Actor::Type, handle2) ||
        !pPolicy->IsHandleObjectAvailable(handle1) ||
        !pPolicy->IsHandleObjectAvailable(handle2))
    {
        return ThisCall(RealCompareVariables, apThis, apVar1, apVar2);
    }

    Actor* pActor1 = pPolicy->GetObjectForHandle<Actor>(handle1);
    Actor* pActor2 = pPolicy->GetObjectForHandle<Actor>(handle2);

    if (!pActor1 || !pActor2)
        return ThisCall(RealCompareVariables, apThis, apVar1, apVar2);

    if (pActor1 == PlayerCharacter::Get())
    {
        auto* pExtension = pActor2->GetExtension();
        if (pExtension && pExtension->IsPlayer())
            return 0;
    }
    else if (pActor2 == PlayerCharacter::Get())
    {
        auto* pExtension = pActor1->GetExtension();
        if (pExtension && pExtension->IsPlayer())
            return 0;
    }

    return ThisCall(RealCompareVariables, apThis, apVar1, apVar2);
#else
    return 0;
#endif
}

static TiltedPhoques::Initializer s_vmHooks([]()
{
    POINTER_SKYRIMSE(TRegisterPapyrusFunction, s_registerPapyrusFunction, 104788);
    POINTER_FALLOUT4(TRegisterPapyrusFunction, s_registerPapyrusFunction, 919894);

    POINTER_SKYRIMSE(TCompareVariables, s_compareVariables, 105220);

    RealRegisterPapyrusFunction = s_registerPapyrusFunction.Get();
    // TODO: ft
#if TP_SKYRIM64
    RealCompareVariables = s_compareVariables.Get();
#endif

    TP_HOOK(&RealRegisterPapyrusFunction, HookRegisterPapyrusFunction);
    TP_HOOK(&RealCompareVariables, HookCompareVariables);
});
