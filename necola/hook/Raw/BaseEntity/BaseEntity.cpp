#include "BaseEntity.h"
#include <spdlog/spdlog.h>

using namespace Hooks;

void __fastcall BaseEntity::SetParents::Detour(C_BaseEntity* pThis, void* edx, C_BaseEntity* pParentEntity, int iParentAttachment)
{
    // FIX: 当实体从世界中分离（pParentEntity == nullptr）且存在于同模共存映射中时，清理映射
    // 这通常在实体即将销毁、离开PVS或被引擎回收时发生
    if (pParentEntity == nullptr && pThis && pThis->entindex() != -1) {
        if (G::WeaponPoly.containsEntity(pThis->entindex())) {
            G::WeaponPoly.removeEntity(pThis->entindex());
            spdlog::debug("[NecolaPolymorphis]BaseEntity::SetParents-> Removed entity {} from poly map on detach", pThis->entindex());
        }
    }

    if(pThis->IsBaseCombatWeapon()) {
        C_BaseCombatWeapon* pCombatWeapon = pThis->MyCombatWeaponPointer();
        if (pCombatWeapon) {
            spdlog::debug("[NecolaPolymorphis]BaseEntity::SetParents-> GetWorldModel:[{}]", pCombatWeapon->GetWorldModel());
        }
    }

    Func.Original<FN>()(pThis, edx, pParentEntity, iParentAttachment);
}

void BaseEntity::Init()
{
    //precompute spherical harmonic coefficients for ambient lighting
    {
        using namespace SetParents;

        const FN pfSetParents = reinterpret_cast<FN>(U::Offsets.m_dwSetParents);
        if( pfSetParents ) {
            Func.Init(pfSetParents, &Detour);
        }

    }

    // FireBullets and TraceAttack GPU command stream intercepts removed - no longer needed
    // DamageShower now uses m_iHealth RecvProxy GPU command stream intercept instead
}
