#pragma once
static void checkSyncPatchLayouts() {
    requireTarget(syncPlayerOffset(0x70e8)==0x70e8,"default finisher layout remains 1.1");
    {
        SyncPatchScope modern(kProfile51);
        requireTarget(syncPlayerOffset(0x70e8)==0x7d78&&syncPlayerOffset(0x15c2c)==0x169d4,"5.1 sync and focus tracker offsets");
        requireTarget(syncEntityOffset(0x6488)==0x6600&&syncEntityOffset(0x1a38)==0x1a68,"5.1 health and sphere physics offsets");
        requireTarget(syncPatchOffset("idActor",0x1c30)==0x1c60&&syncPatchOffset("idSyncParticipant",0xaa)==0xac,"interior and participant offsets are typed");
        requireTarget(syncPatchOffset("idSyncEntity",0x2bd0)==0x2c10&&syncPatchOffset("idSyncEntity",0x2c08)==0x2c48,"adjacent fields map independently");
        requireTarget(syncPatchOffset("idDeclAnimWeb",0x78)==0x80&&syncPatchOffset("idJointAnimator",0)==8,"5.1 animator and web fields");
        requireTarget(syncPatchSize("idPlayer",0x70e8,0x628)==0x650,"reflection sizes follow their patch");
        std::vector<unsigned char> declaration(0x400);bool ordinary=false;
        requireTarget(syncOrdinarySpecialFlags(WallMemory{},targetAddress(declaration),ordinary)&&ordinary,"ordinary 5.1 declaration");
        targetPut(declaration,0x32e,static_cast<unsigned char>(1));
        requireTarget(syncOrdinarySpecialFlags(WallMemory{},targetAddress(declaration),ordinary)&&ordinary,"5.1 Blood Punch reward flag does not suppress ordinary GK");
        targetPut(declaration,0x32e,static_cast<unsigned char>(0));targetPut(declaration,0x32f,static_cast<unsigned char>(1));
        requireTarget(syncOrdinarySpecialFlags(WallMemory{},targetAddress(declaration),ordinary)&&!ordinary,"5.1 momentum flag is read after Blood Punch");
        {SyncPatchScope legacy(kProfile11);requireTarget(syncPlayerOffset(0x70e8)==0x70e8,"nested 1.1 fixture has its own layout");}
        requireTarget(syncPlayerOffset(0x70e8)==0x7d78,"nested scope restores 5.1");
        requireTarget(!syncPatchCodeMatches(WallMemory{},0),"missing patch instructions disable modern preview");
    }
    requireTarget(syncPlayerOffset(0x70e8)==0x70e8,"scope restores 1.1 without leaking between tests");
    {
        SyncPatchScope patch31(kProfile31);
        requireTarget(syncPlayerOffset(0x70e8)==0x7c90&&syncEntityOffset(0x6488)==0x6498,
            "3.1 uses its own sync attack and death flag");
        requireTarget(syncPatch().emptyEquipment==0x2787fa0,"3.1 uses its own ordinary equipment sentinel");
    }
    for(const auto* profile:kGameProfiles)
        requireTarget(syncPreviewProfile(*profile),"every supported game patch has a finisher layout");
    auto altered=kProfile31;
    ++altered.imageSize;
    requireTarget(!syncPatchFor(altered),"a modified PE identity cannot select a finisher layout");
    puts("PASS: full finisher patch registry, scoped layouts, special requirements and code guards.");
}
