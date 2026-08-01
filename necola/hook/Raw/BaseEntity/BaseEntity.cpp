#include "BaseEntity.h"
#include <spdlog/spdlog.h>

using namespace Hooks;

void __fastcall BaseEntity::SetParents::Detour(C_BaseEntity* pThis, void* edx, C_BaseEntity* pParentEntity, int iParentAttachment)
{
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

    Func.Original()(pThis, edx, pParentEntity, iParentAttachment);
}

void BaseEntity::Init()
{
    {
        using namespace SetParents;

        const FN pfSetParents = reinterpret_cast<FN>(U::Offsets.m_dwSetParents);
        if( pfSetParents ) {
            Func.Init(pfSetParents, &Detour);
        }

    }
}
