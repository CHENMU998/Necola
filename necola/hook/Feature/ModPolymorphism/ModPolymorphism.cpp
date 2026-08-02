#include "ModPolymorphism.h"
#include "../../Vars.h"
#include <spdlog/spdlog.h>
#include <string>

void ModPolymorphism::clear() {
    G::WeaponPoly.clearEntity();
    G::WeaponPoly.clearPrecache();
    G::WeaponPoly.clearModelIndex();
}

void ModPolymorphism::clearWhenMisssonLost() {
    G::WeaponPoly.clearEntity();
    G::WeaponPoly.clearPrecache();
}

void ModPolymorphism::LoadModel() {
    bool loadPolyInfo = G::WeaponPoly.initializeFromJsonFile("modPolymorphismData.json");
    if(!loadPolyInfo) {
        spdlog::info("[ModelPolymorphis]LevelInitPreEntity->initializeFromJsonFile fail. feature disabled.");
        G::Vars.enableModelPolymorphism = false;
    } else {
        INetworkStringTable* pModelPrecache = I::NetworkStringTable->FindTable("modelprecache");
        if(pModelPrecache) {
            for(auto it = G::WeaponPoly.weaponPolymorphisCount.begin(); it != G::WeaponPoly.weaponPolymorphisCount.end(); ++it) {
                int weaponID = it->first;
                if(G::WeaponPoly.hasModelName(weaponID)) {
                    const std::vector<std::string> modelNames = G::WeaponPoly.getModelNames(weaponID);
                    for(std::string name: modelNames) {
                        int sourceModexIndex = I::ModelInfo->GetModelIndex(name.c_str());
                        if(sourceModexIndex > 0) {
                            for(int polyNum = 1; polyNum <= it->second ; polyNum ++) {
                                const char* necolaModelName = G::Util.convertToNecolaModelName(name.c_str(), polyNum);
                                const model_t* modelt = I::ModelInfo->FindOrLoadModel(necolaModelName);
                                if(modelt) {
                                    pModelPrecache->AddString(false, necolaModelName);
                                } else {
                                    spdlog::info("LevelInitPreEntity I::ModelInfo->FindOrLoadModel [{}] fail.", necolaModelName);
                                }

                                int necolaModelIndex = I::ModelInfo->GetModelIndex(necolaModelName);
                                if(necolaModelIndex > 0) {
                                    G::WeaponPoly.addModelIndex(sourceModexIndex, polyNum, necolaModelIndex);
                                    G::WeaponPoly.addModelIndex(necolaModelIndex, 0, sourceModexIndex);
                                } else {
                                    spdlog::info("LevelInitPreEntity I::ModelInfo->GetModelIndex [{}] fail. Skip adding fallback self-mapping.", necolaModelName);
                                    // FIX: 移除自映射回退，避免映射表污染
                                    // 当necola模型加载失败时，不应将sourceIndex映射到自身，
                                    // 这样getPolyIndexBySourceIndex会自然回退到原始modelIndex
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void ModPolymorphism::CacheEntityPolyInVGui() {
    if (!I::EngineClient->IsInGame()) {
        return;
    }
    for (int n = 1; n < (I::ClientEntityList->GetMaxEntities() + 1); n++) {
        C_BaseEntity* pEntity = I::ClientEntityList->GetClientEntity(n)->As();
        if (!pEntity || pEntity->IsDormant()) {
            continue;
        }
        int entIndex = pEntity->entindex();
        if(entIndex == -1 || G::WeaponPoly.containsEntity(entIndex)) {
            continue;
        }
        ClientClass* pCC = pEntity->GetClientClass();
        if(!pCC) {
            continue;
        }
        switch(pCC->m_ClassID) {
            case CTerrorMeleeWeapon:
            {
                const model_t* model = pEntity->GetModel();
                if(model) {
                    int weaponId = G::Util.getWeaponIDWithSubtype(I::ModelInfo->GetModelName(model));
                    if(G::WeaponPoly.hasPolymorphis(weaponId)) {
                        int poly = G::WeaponPoly.selectPolymorphism(weaponId);

                        C_BaseCombatWeapon* pCombatWeapon = pEntity->MyCombatWeaponPointer();
                        if(pCombatWeapon) {
                            C_BaseCombatCharacter* weaponOwner = pCombatWeapon->m_hOwner()->As();
                            if(weaponOwner) {
                                int playerEntityId = weaponOwner->entindex();
                                if(G::WeaponPoly.hasPrecache(playerEntityId, weaponId)) {
                                    poly = G::WeaponPoly.popPrecache(playerEntityId, weaponId);
                                }
                            }
                        }

                        G::WeaponPoly.addEntity(entIndex, poly);
                    }
                }
                break;
            }
            case CWeaponSpawn:
            {
                C_WeaponSpawn* weaponSpawnEntity = pEntity->As<C_WeaponSpawn>();
                int weaponId = weaponSpawnEntity->GetWeaponID();
                if(weaponId == NECOLA_WEAPON_MELEE) {
                    const model_t* model = pEntity->GetModel();
                    if(model) {
                        weaponId = G::Util.getWeaponIDWithSubtype(I::ModelInfo->GetModelName(model));
                    }
                }
                if(G::WeaponPoly.hasPolymorphis(weaponId)){
                    int poly = G::WeaponPoly.selectPolymorphism(weaponId);
                    G::WeaponPoly.addEntity(entIndex, poly);
                }
                break;
            }
            case CBaseUpgradeItem:
            {
                const model_t* model = pEntity->GetModel();
                if(model && model->szName) {
                    size_t nameLen = strlen(model->szName);
                    // FIX: 添加字符串长度检查，防止越界访问
                    if (nameLen > 20 && model->szName[0] == 'm') {
                        if (model->szName[7] == 'p' && model->szName[13] == 't') {
                            if (model->szName[20] == 'e') {
                                if(G::WeaponPoly.hasPolymorphis(NECOLA_WEAPON_UPGRADEPACK_EXPLOSIVE)) {
                                    int poly = G::WeaponPoly.selectPolymorphism(NECOLA_WEAPON_UPGRADEPACK_EXPLOSIVE);
                                    G::WeaponPoly.addEntity(entIndex, poly);
                                }
                            }
                            else if(model->szName[20] == 'i') {
                                if(G::WeaponPoly.hasPolymorphis(NECOLA_WEAPON_UPGRADEPACK_INCENDIARY)) {
                                    int poly = G::WeaponPoly.selectPolymorphism(NECOLA_WEAPON_UPGRADEPACK_INCENDIARY);
                                    G::WeaponPoly.addEntity(entIndex, poly);
                                }
                            }
                        }
                        else if (nameLen > 32 && model->szName[7] == 'w' && model->szName[26] == 'l' && model->szName[32] == 's') {
                            if(G::WeaponPoly.hasPolymorphis(NECOLA_LASER)) {
                                int poly = G::WeaponPoly.selectPolymorphism(NECOLA_LASER);
                                G::WeaponPoly.addEntity(entIndex, poly);
                            }
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void ModPolymorphism::CacheEntityPolyInCreated(C_BaseEntity* pEntity) {
    if(!pEntity || pEntity->entindex() == -1) {
        return;
    }

    int entIndex = pEntity->entindex();

    if(pEntity->IsBaseCombatWeapon()) {
        C_BaseCombatWeapon* pCombatWeapon = pEntity->MyCombatWeaponPointer();
        if(pCombatWeapon) {
            ClientClass* pCC = pEntity->GetClientClass();
            if(pCC) {
                int currentWeaponId = G::Util.entityClassID2WeaponID(pCC->m_ClassID);
                if(G::WeaponPoly.hasPolymorphis(currentWeaponId)){
                    int poly = G::WeaponPoly.selectPolymorphism(currentWeaponId);
                    G::WeaponPoly.addEntity(entIndex, poly);
                }
            }
        }
    } else {
        ClientClass* pCC = pEntity->GetClientClass();
        if (pCC) {
            switch(pCC->m_ClassID) {
                case CWeaponAmmoSpawn:
                {
                    if(G::WeaponPoly.hasPolymorphis(NECOLA_AMMOSPAWN)) {
                        int poly = G::WeaponPoly.selectPolymorphism(NECOLA_AMMOSPAWN);
                        G::WeaponPoly.addEntity(entIndex, poly);
                    }
                    break;
                }
                case CMolotovProjectile:
                {
                    if(G::WeaponPoly.hasPolymorphis(NECOLA_WEAPON_MOLOTOV)) {
                        int poly = G::WeaponPoly.selectPolymorphism(NECOLA_WEAPON_MOLOTOV);
                        G::WeaponPoly.addEntity(entIndex, poly);
                    }
                    break;
                }
                case CPipeBombProjectile:
                {
                    if(G::WeaponPoly.hasPolymorphis(NECOLA_WEAPON_PIPEBOMB)) {
                        int poly = G::WeaponPoly.selectPolymorphism(NECOLA_WEAPON_PIPEBOMB);
                        G::WeaponPoly.addEntity(entIndex, poly);
                    }
                    break;
                }
                case CVomitJarProjectile:
                {
                    if(G::WeaponPoly.hasPolymorphis(NECOLA_WEAPON_VOMITJAR)) {
                        int poly = G::WeaponPoly.selectPolymorphism(NECOLA_WEAPON_VOMITJAR);
                        G::WeaponPoly.addEntity(entIndex, poly);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void ModPolymorphism::CacheEntityPolyInSpawnerGiveItem(int userid, const char* itemName, int spawner) {
    if(G::WeaponPoly.containsEntity(spawner)) {
        int poly = G::WeaponPoly.getPolyByEntityID(spawner);
        int weaponId = G::Util.getWeaponIDByWeaponName(itemName);
        if(weaponId != -1) {
            if(strcmp(itemName, "weapon_melee") == 0) {
                C_BaseEntity* spawnerEntity = I::ClientEntityList->GetClientEntity(spawner)->As<C_BaseEntity>();
                if(spawnerEntity) {
                    int modelIndex = spawnerEntity->m_nModelIndex();
                    const char* modelName = I::ModelInfo->GetModelName(I::ModelInfo->GetModel(modelIndex));
                    if(modelName) {
                        weaponId = G::Util.getWeaponIDWithSubtype(modelName);
                    }
                }
            }
            if(weaponId != -1) {
                int userEntityIndex = I::EngineClient->GetPlayerForUserID(userid);
                if(userEntityIndex != -1) {
                    G::WeaponPoly.addPrecache(userEntityIndex, weaponId, poly);
                }
            }
        }
    }
}

// FIX: 核心修复 - 添加实体有效性验证、类型检查和空指针保护
void ModPolymorphism::ModifyEntityPolyPerFrame() {
    for(auto it = G::WeaponPoly.entityPolymorphis.begin(); it != G::WeaponPoly.entityPolymorphis.end(); ) {
        int entIndex = it->first;
        C_BaseEntity* pEntity = I::ClientEntityList->GetClientEntity(entIndex)->As<C_BaseEntity>();

        // FIX 1: 实体已销毁，立即清理映射，防止entindex复用污染
        if (!pEntity) {
            spdlog::debug("[NecolaPolymorphis]ModifyEntityPolyPerFrame: Entity {} destroyed, removing from poly map.", entIndex);
            it = G::WeaponPoly.entityPolymorphis.erase(it);
            continue;
        }

        // FIX 2: 验证实体类型，确保entindex没有被复用给其他类型实体
        ClientClass* pCC = pEntity->GetClientClass();
        if (!pCC) {
            ++it;
            continue;
        }

        bool isValidPolyTarget = false;
        switch(pCC->m_ClassID) {
            case CTerrorMeleeWeapon:
            case CWeaponSpawn:
            case CBaseUpgradeItem:
            case CWeaponAmmoSpawn:
            case CMolotovProjectile:
            case CPipeBombProjectile:
            case CVomitJarProjectile:
                isValidPolyTarget = true;
                break;
            default:
                break;
        }

        if (!isValidPolyTarget) {
            // entindex已被引擎复用给非武器/物品实体（如墙壁、人物）
            spdlog::debug("[NecolaPolymorphis]ModifyEntityPolyPerFrame: Entity {} classID {} mismatch, removing from poly map.", entIndex, pCC->m_ClassID);
            it = G::WeaponPoly.entityPolymorphis.erase(it);
            continue;
        }

        if(pEntity->IsBaseCombatWeapon()){
            C_BaseCombatWeapon* pCombatWeapon = pEntity->MyCombatWeaponPointer();
            if(pCombatWeapon) {
                int poly = it->second;
                if(pCombatWeapon->m_hOwner()) {
                    int currentWeaponId = G::Util.entityClassID2WeaponID(pCC->m_ClassID);
                    if(currentWeaponId != NECOLA_WEAPON_MELEE) {
                        C_BaseCombatCharacter* weaponOwner = pCombatWeapon->m_hOwner()->As<C_BaseCombatCharacter>();
                        if(weaponOwner) {
                            int playerEntityId = weaponOwner->entindex();
                            if(G::WeaponPoly.hasPrecache(playerEntityId, currentWeaponId)) {
                                poly = G::WeaponPoly.popPrecache(playerEntityId, currentWeaponId);
                                G::WeaponPoly.addEntity(entIndex, poly);
                            }
                        }
                    }
                }
                if(poly > 0 ) {
                    int sourceIViewModelIndex = pCombatWeapon->m_iViewModelIndex();
                    int sourceIWorldModelIndex = pCombatWeapon->m_iWorldModelIndex();
                    int sourceNModelIndex = pCombatWeapon->m_nModelIndex();

                    int newIViewModelIndex = G::WeaponPoly.getPolyIndexBySourceIndex(sourceIViewModelIndex, poly);
                    int newIWorldModelIndex = G::WeaponPoly.getPolyIndexBySourceIndex(sourceIWorldModelIndex, poly);
                    int newNModelIndex = G::WeaponPoly.getPolyIndexBySourceIndex(sourceNModelIndex, poly);

                    pCombatWeapon->m_iWorldModelIndex() = newIWorldModelIndex;
                    pCombatWeapon->m_iViewModelIndex() = newIViewModelIndex;
                    pCombatWeapon->m_nModelIndex() = newNModelIndex;
                }
            }
        } else {
            int poly = it->second;
            if(poly > 0) {
                int modelIndex = pEntity->m_nModelIndex();
                int newModelIndex = G::WeaponPoly.getPolyIndexBySourceIndex(modelIndex, poly);
                if(modelIndex != newModelIndex) {
                    pEntity->m_nModelIndex() = newModelIndex;

                    // FIX 3: 空指针保护 - 确保模型有效后再调用SetModelInternalOffset
                    const model_t* pNewModel = I::ModelInfo->GetModel(newModelIndex);
                    if (pNewModel) {
                        const char* szModelName = I::ModelInfo->GetModelName(pNewModel);
                        if (szModelName && szModelName[0] != '\0') {
                            pEntity->SetModelInternalOffset(szModelName);
                        }
                    }
                }
            }
        }
        ++it;
    }
}

// FIX: 添加空指针保护
void ModPolymorphism::ModifyLocalPlayerViewModel() {
    C_TerrorPlayer* pLocal = I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer())->As<C_TerrorPlayer>();
    if( pLocal && !pLocal->deadflag()) {
        C_TerrorWeapon* pWeapon = pLocal->GetActiveWeapon()->As<C_TerrorWeapon>();
        if(pWeapon) {
            C_BaseViewModel* pViewModel = static_cast<C_BaseViewModel*>(pLocal->m_hViewModel());
            if(pViewModel) {
                int sourceIViewModelIndex = pWeapon->m_iViewModelIndex();
                if(pWeapon->GetWeaponID() == NECOLA_WEAPON_PISTOL){
                    sourceIViewModelIndex = pWeapon->m_nModelIndex();
                }
                int sourceViewModelIndex = pViewModel->m_nModelIndex();

                if(sourceViewModelIndex != sourceIViewModelIndex) {
                    pViewModel->m_nModelIndex() = sourceIViewModelIndex;
                }

                // FIX: 空指针保护
                const model_t* pTargetModel = I::ModelInfo->GetModel(sourceIViewModelIndex);
                if(pViewModel->GetModel() && pTargetModel) {
                    if(pViewModel->GetModel() != pTargetModel) {
                        const char* szModelName = I::ModelInfo->GetModelName(pTargetModel);
                        if(szModelName) {
                            pViewModel->SetModelInternalOffset(szModelName);
                        }
                    }
                }
            }
        }
    }
}
