#pragma once

static void checkBodyPaletteByOccupiedSector() {
    const auto enemies=loadCapturedLimbEnemies();size_t actors=0,cases=0,halves=0,anatomical=0,visibleParts=0;
    std::ofstream report("build/body-sectors-v23-coverage.tsv"),atlas("build/body-sectors-v23-atlas.tsv");atlas.precision(9);
    report<<"enemy\tside45\tpalette\tupper\tlower\tparts\n";
    for(const auto& enemy:enemies){std::string blueprint,family;
        if(enemy.definition=="ai/fodder/zombie_tier_1"){blueprint="zombie_tier1";family="zombie_tier1";}
        else if(enemy.definition=="ai/fodder/zombie_t1_hell"){blueprint="zombie_tier1_hell";family="zombie_tier1";}
        else if(enemy.definition=="ai/fodder/zombie_t1_scientist"){blueprint="zombie_tier1_scientist";family="zombie_tier1";}
        else if(enemy.definition=="ai/fodder/soldier_blaster"){blueprint="soldier_blaster";family="soldier_blaster";}
        else if(enemy.definition=="ai/fodder/soldier_shield"){blueprint="soldier_shield";family="soldier_blaster";}
        else if(enemy.definition!="ai/fodder/zombie_tier_3")continue;
        auto frame=enemy.frame;
        if(blueprint.empty()){GameCamera camera;const auto geometry=loadRightArmPose(camera);frame.regions=geometry.regions;}
        else frame.regions=loadCapturedZombieSoldierFrame("syncmelee/"+blueprint,family).regions;
        frame.selectedAnimation.clear();frame.regionState="ready";frame.ranges.known=true;frame.ranges.gloryKillTarget=10;frame.ranges.lunge=10;
        ++actors;
        for(unsigned side=0;side<8;++side){auto camera=limbOrbitCamera(frame,side,false);
            // Same native actor/pose from eight player positions. Keep real
            // ground height: the slight fixture floor mismatch must not
            // accidentally admit the aerial unrestricted interactions.
            frame.ranges.playerOrigin={camera.position[0],camera.position[1],frame.facing.origin.z};
            const auto scopes=syncVisibleAimRequirements(frame);requireTarget(!scopes.empty(),"recorded Zombie/Soldier has actual ground rules in every tested direction");
            bool named=false,upper=false,lower=false;
            for(const auto& aim:scopes){
                if(!aim.damageGroups.empty()){bool accepted=false,excluded=false;
                    for(const auto& s:frame.spheres)if(!s.focusGroup.empty()){
                        const bool matches=std::find(aim.damageGroups.begin(),aim.damageGroups.end(),s.focusGroup)!=aim.damageGroups.end();accepted|=matches;excluded|=!matches;}
                    named|=accepted&&excluded;
                }
                upper|=aim.quadrants==3;lower|=aim.quadrants==12;
            }
            const auto detail=syncTargetAimDetail(frame,scopes);
            if(!named&&upper&&lower){++halves;
                // Fails in v21: global front limb capability forces Anatomy
                // even though no named-part rule exists at this side/back.
                requireTarget(detail==SyncAimDetail::Halves,"side/back upper/lower sectors must override the enemy's front-only anatomical capability");
            }else if(named){++anatomical;requireTarget(detail==SyncAimDetail::Anatomy,"front sectors with available named parts retain detailed anatomy");}
            const auto mesh=syncBuildTargetAimMesh(camera,frame);++cases;
            if(!named&&upper&&lower)requireTarget(mesh.fill.empty()&&!mesh.quadrantFill.empty()&&!mesh.lowerFill.empty(),"both blue upper and purple lower halves must actually render from side/back");
            std::unordered_set<std::string> drawnGroups;
            if(named)for(const auto* fill:{&mesh.fill,&mesh.quadrantFill,&mesh.lowerFill})for(size_t i=0;i<fill->size();i+=3){double x=0,y=0,w;
                for(unsigned j=0;j<3;++j){double px,py;requireTarget(syncAimProject(camera,(*fill)[i+j],px,py,w),"individual-part vertex projects");x+=px/3;y+=py/3;}
                if(const auto* hit=bodyColourFirstHit(camera,frame,x,y))drawnGroups.insert(hit->focusGroup);
            }
            for(const auto& aim:scopes)for(const auto& group:aim.damageGroups){auto specific=aim;specific.damageGroups={group};
                if(limbVisibleCore(camera,frame,specific)){++visibleParts;requireTarget(drawnGroups.count(group)!=0,"every separately available visible native body part must survive the complete multi-animation sector overlay");}
            }
            report<<enemy.definition<<'\t'<<side<<'\t'<<int(detail)<<'\t'<<mesh.quadrantFill.size()/3<<'\t'<<mesh.lowerFill.size()/3<<'\t'<<mesh.fill.size()/3<<'\n';
            for(unsigned channel=0;channel<3;++channel){const auto& fill=channel==0?mesh.fill:channel==1?mesh.quadrantFill:mesh.lowerFill;
                for(size_t i=0;i<fill.size();i+=3){atlas<<enemy.definition<<'\t'<<side<<'\t'<<channel;
                    for(unsigned j=0;j<3;++j){double x,y,w;requireTarget(syncAimProject(camera,fill[i+j],x,y,w),"sector colour vertex projects");atlas<<'\t'<<x<<'\t'<<y;}atlas<<'\n';}
            }
        }
    }
    requireTarget(actors==6&&cases==48&&halves>=18&&anatomical>=6,"all six recorded Zombie/Soldier variants cover front, side and back palette transitions");
    requireTarget(visibleParts>=12,"native multi-part preservation has real visibly exposed part cases");
    printf("PASS: occupied-sector body palettes: %zu actors, %zu directions, %zu upper/lower sectors, %zu anatomical sectors, %zu individually visible part rules preserved.\n",actors,cases,halves,anatomical,visibleParts);
}
