#pragma once

static const SyncHitSphere* bodyColourFirstHit(const GameCamera& camera,const SyncTargetFrame& frame,double x,double y) {
    Vec3 point{};if(!syncAimUnproject(camera,x,y,1,point))return nullptr;
    const Vec3 eye{camera.position[0],camera.position[1],camera.position[2]},ray=subtract(point,eye);
    double nearest=DBL_MAX;const SyncHitSphere* first=nullptr;
    for(const auto& sphere:frame.spheres){const auto d=subtract(eye,sphere.center);const double a=dot(ray,ray),b=dot(d,ray),c=dot(d,d)-double(sphere.radius)*sphere.radius;
        const double disc=b*b-a*c;if(disc<0||a<=0)continue;const double root=sqrt(disc),entry=(-b-root)/a,leave=(-b+root)/a,t=entry>=0?entry:leave;
        if(t>=0&&t<nearest){nearest=t;first=&sphere;}
    }
    return first;
}
static void checkBodyColourScopes() {
    double entry,leave;
    requireTarget(!syncAimPartInterval({0,0,0},{1,0,0},SyncHitSphere{{.299f,0,0},.3f,"chest"},entry,leave),"camera inside the true sphere skin cannot invent a nearer inset hit of another body colour");
    SyncTargetFrame frame;frame.selectedAnimation="scope";frame.aim.known=true;
    frame.spheres={{{0,4,1},.28f,"",1,true,"head"},{{0,4,0},.3f,"",2,true,"chest"},
        {{-.7f,4,-.8f},.24f,"",3,true,"left_leg"},{{.7f,4,.5f},.24f,"",4,true,"right_arm"},{{0,4.6f,0},.2f,"",5,true,"back"}};
    syncShotBounds(frame.spheres,frame.shotBounds);memcpy(frame.aim.bounds,frame.shotBounds,24);
    frame.animationRules={{"scope",0,{0,true,{"head","chest","left_leg","right_arm"}},{}},{"head",0,{3,true,{"head"}}, {}}};
    GameCamera camera{};camera.matrix[0]=camera.matrix[6]=camera.matrix[13]=1;
    auto mesh=syncBuildTargetAimMesh(camera,frame);
    requireTarget(!mesh.fill.empty()&&!mesh.quadrantFill.empty()&&!mesh.lowerFill.empty(),"named arms/head are orange, torso blue and anatomical legs purple");
    bool head=false,arm=false,leg=false,torso=false;
    for(const auto& channel:{std::make_pair(&mesh.fill,0),std::make_pair(&mesh.quadrantFill,1),std::make_pair(&mesh.lowerFill,2)})
        for(size_t i=0;i<channel.first->size();i+=3){double x=0,y=0,w;
            for(unsigned j=0;j<3;++j){double px,py;requireTarget(syncAimProject(camera,(*channel.first)[i+j],px,py,w),"detail vertex projects");x+=px/3;y+=py/3;}
            const auto* hit=bodyColourFirstHit(camera,frame,x,y);requireTarget(hit,"detail cannot colour empty space");
            if(channel.second==0){requireTarget(hit->focusGroup=="head"||hit->focusGroup=="right_arm","orange cannot bleed onto torso or legs");head|=hit->focusGroup=="head";arm|=hit->focusGroup=="right_arm";}
            else if(channel.second==1){requireTarget(hit->focusGroup=="chest","torso blue cannot cover an exposed head, arm or leg");torso=true;}
            else {requireTarget(hit->focusGroup=="left_leg","anatomical legs use purple even in front");leg=true;}
        }
    requireTarget(head&&arm&&leg&&torso,"every exposed synthetic body class is represented individually");
    auto chest=frame;chest.animationRules[0].aim.damageGroups={"chest"};syncRefreshAimMesh(camera,chest);
    const auto lines=syncPositionGuideLines(chest);
    requireTarget(std::find(lines.begin(),lines.end(),"Aim at a highlighted body area")!=lines.end(),"visible blue torso guidance cannot be mislabeled hidden just because it has no orange vertices");
    const auto detailedSignature=syncAimMeshSignature(camera,frame,"scope");
    frame.animationRules[0].aim.damageGroups.clear();
    requireTarget(detailedSignature!=syncAimMeshSignature(camera,frame,"scope"),"changing the selected native partition invalidates the mesh even with the same animation name");
    frame.animationRules[0].aim.quadrants=15;mesh=syncBuildTargetAimMesh(camera,frame);
    requireTarget(mesh.fill.empty()&&!mesh.quadrantFill.empty()&&!mesh.lowerFill.empty(),"unrestricted body is blue with actual anatomical legs consistently purple");
    frame.animationRules[0].aim.quadrants=3;auto upper=syncBuildTargetAimMesh(camera,frame);
    frame.animationRules[0].aim.quadrants=12;auto lower=syncBuildTargetAimMesh(camera,frame);
    requireTarget(upper.fill.empty()&&!upper.quadrantFill.empty()&&upper.lowerFill.empty()&&lower.fill.empty()&&lower.quadrantFill.empty()&&!lower.lowerFill.empty(),"quadrant-only upper is blue and lower is purple");
    const std::vector<SyncAimRequirement> scopes{{3,true,{"head"}},{12,true,{"left_leg"}}};
    const auto disjoint=syncDisjointAimRequirements(scopes);
    for(const auto& scope:disjoint)requireTarget((scope.quadrants==3&&scope.damageGroups==std::vector<std::string>{"head"})||(scope.quadrants==12&&scope.damageGroups==std::vector<std::string>{"left_leg"}),"scope deduplication cannot invent lower head or upper leg targeting");
    frame.animationRules={{"scope",0,{0,true,{"head"}}, {}}};
    const auto coarse=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&frame.animationRules[0].aim,frame.spheres);
    const auto fine=syncBuildTargetAimMesh(camera,frame);
    requireTarget(fine.fill.size()>coarse.fill.size()*1.5,"detail mesh has substantially more real head triangles than the previous grid");
    printf("PASS: body colours, exposed head/arms/legs/torso, native half split, disjoint scopes, cache and increased detail.\n");
}

static void checkCapturedBodyColours() {
    const auto enemies=loadCapturedLimbEnemies();size_t models=0,cases=0,triangles=0,orange=0,blue=0,purple=0;
    const std::unordered_set<std::string> anatomicalFamilies{"imp","carcass","gargoyle","mancubus_fire","prowler","soldier_blaster","zombie_tier1","zombie_tier3"};
    const std::unordered_set<std::string> appendages{"head","left_arm","right_arm","left_arm_mech","right_arm_mech","left_upper_arm","right_upper_arm","left_lower_arm","right_lower_arm",
        "left_leg","right_leg","rear_leg","left_upper_leg","right_upper_leg","left_lower_leg","right_lower_leg","left_foot","right_foot","left_wing","right_wing"};
    std::ofstream report("build/body-colours-v23-coverage.tsv"),atlas("build/body-colours-v23-atlas.tsv");atlas.precision(9);
    report<<"enemy\tstate\taction\tside\tclose\tanatomy\torange\tblue\tpurple\n";
    for(const auto& enemy:enemies){auto frame=enemy.frame;if(!enemy.shape||!enemy.pose||!frame.aim.known)continue;++models;
        bool capable=false;std::vector<SyncAimRequirement> allScopes;
        for(const auto& rule:frame.animationRules){capable|=anatomicalFamilies.count(rule.audit.family)!=0;allScopes.push_back(rule.aim);}
        requireTarget(syncHasNamedBodyAim(frame,allScopes)==capable,"all captured native model variants retain their reviewed anatomical capabilities");
        for(const auto& rule:frame.animationRules)for(unsigned view=0;view<6;++view){const unsigned side=view<4?view*2:(view-4)*4;const bool close=view>=4;
            bool accepted=false,excluded=false;
            if(!rule.aim.damageGroups.empty())for(const auto& sphere:frame.spheres)if(!sphere.focusGroup.empty()){
                const bool match=std::find(rule.aim.damageGroups.begin(),rule.aim.damageGroups.end(),sphere.focusGroup)!=rule.aim.damageGroups.end();accepted|=match;excluded|=!match;}
            const bool anatomy=accepted&&excluded;
            frame.action=rule.action;frame.selectedAnimation=rule.name;const auto camera=limbOrbitCamera(frame,side,close);const auto mesh=syncBuildTargetAimMesh(camera,frame);++cases;
            if(frame.action==1){requireTarget(mesh.known&&mesh.fill.empty()&&mesh.quadrantFill.empty()&&mesh.lowerFill.empty(),"all captured Chainsaw models/views stay unpainted");continue;}
            const bool halves=!syncAimUnrestricted(rule.aim.quadrants)&&!(rule.aim.quadrants&3);double cx=0,cy=0,w;
            const auto* b=frame.aim.bounds;if(halves)requireTarget(syncAimProject(camera,{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},cx,cy,w),"captured native split projects");
            report<<enemy.definition<<'\t'<<rule.name<<'\t'<<rule.action<<'\t'<<side<<'\t'<<close<<'\t'<<anatomy<<'\t'<<mesh.fill.size()/3<<'\t'<<mesh.quadrantFill.size()/3<<'\t'<<mesh.lowerFill.size()/3<<'\n';
            orange+=mesh.fill.size()/3;blue+=mesh.quadrantFill.size()/3;purple+=mesh.lowerFill.size()/3;
            for(unsigned channel=0;channel<3;++channel){const auto& fill=channel==0?mesh.fill:channel==1?mesh.quadrantFill:mesh.lowerFill;
                auto expected=rule.aim;expected.damageGroups.clear();uint32_t mask=syncAimUnrestricted(rule.aim.quadrants)?15:rule.aim.quadrants;
                for(const auto& sphere:frame.spheres){const bool acceptsSphere=rule.aim.damageGroups.empty()||std::find(rule.aim.damageGroups.begin(),rule.aim.damageGroups.end(),sphere.focusGroup)!=rule.aim.damageGroups.end();
                    const bool leg=sphere.focusGroup.find("leg")!=std::string::npos||sphere.focusGroup.find("foot")!=std::string::npos;
                    const unsigned expectedColour=leg?2u:anatomy&&appendages.count(sphere.focusGroup)?0u:halves?2u:1u;
                    const bool colour=channel==expectedColour;
                    if(acceptsSphere&&colour&&std::find(expected.damageGroups.begin(),expected.damageGroups.end(),sphere.focusGroup)==expected.damageGroups.end())expected.damageGroups.push_back(sphere.focusGroup);
                }
                expected.quadrants=mask;
                if(mask&&!expected.damageGroups.empty()&&limbVisibleCore(camera,frame,expected)){
                    if(fill.empty()){
                        const auto palette=syncTargetAimColourRules(frame,{rule.aim});
                        const auto focused=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&expected,frame.spheres,true,true,SyncAimDetail::Anatomy,&palette,false);
                        const auto plain=syncBuildAimPartMesh(camera,frame.shotBounds,frame.aim,&expected,frame.spheres);
                        fprintf(stderr,"MISSING COLOUR %s %s side=%u close=%d channel=%u anatomy=%d mask=%u direct=%zu/%zu/%zu known=%d plain=%zu groups=%s\n",enemy.definition.c_str(),rule.name.c_str(),side,close,channel,anatomy,mask,focused.fill.size(),focused.quadrantFill.size(),focused.lowerFill.size(),focused.known,plain.fill.size(),syncAimGroupKey(expected.damageGroups).c_str());
                    }
                    requireTarget(!fill.empty(),"every independently visible body colour class survives on every tested native model/view");}
                for(size_t i=0;i<fill.size();i+=3){++triangles;double x[3],y[3];
                    for(unsigned j=0;j<3;++j)requireTarget(syncAimProject(camera,fill[i+j],x[j],y[j],w),"captured coloured vertex projects");
                    for(const auto& bary:{std::array<double,3>{1./3,1./3,1./3},std::array<double,3>{.7,.15,.15}}){
                        const double px=x[0]*bary[0]+x[1]*bary[1]+x[2]*bary[2],py=y[0]*bary[0]+y[1]*bary[1]+y[2]*bary[2];
                        const auto* hit=bodyColourFirstHit(camera,frame,px,py);requireTarget(hit,"every coloured triangle hits the actual captured body first");
                        requireTarget(rule.aim.damageGroups.empty()||std::find(rule.aim.damageGroups.begin(),rule.aim.damageGroups.end(),hit->focusGroup)!=rule.aim.damageGroups.end(),"colours never expand the native accepted joint groups");
                        if(!syncAimUnrestricted(rule.aim.quadrants))requireTarget(limbIndependentRay(camera,frame,rule.aim.damageGroups.empty()?SyncAimRequirement{rule.aim.quadrants,true,{hit->focusGroup}}:rule.aim,px,py),"coloured body remains within the native screen quadrants");
                        const bool leg=hit->focusGroup.find("leg")!=std::string::npos||hit->focusGroup.find("foot")!=std::string::npos;
                        const unsigned expectedColour=leg?2u:anatomy&&appendages.count(hit->focusGroup)?0u:halves?2u:1u;
                        requireTarget(channel==expectedColour,"all native scopes keep legs purple and distinguish named parts from lower-screen animation areas");
                    }
                    const bool preview=rule.action==0&&((anatomy&&syncAimUnrestricted(rule.aim.quadrants)&&rule.aim.damageGroups.empty())||(!anatomy&&rule.aim.quadrants==12));
                    if(view==0&&preview){atlas<<enemy.definition<<'\t'<<rule.name<<'\t'<<channel;for(unsigned j=0;j<3;++j)atlas<<'\t'<<x[j]<<'\t'<<y[j];atlas<<'\n';}
                }
            }
        }
    }
    report.flush();atlas.flush();
    requireTarget(models==26&&cases>1500&&orange>1000&&blue>1000&&purple>1000,"captured colour regression covers every usable spawned model and all three requested colours");
    printf("PASS: body colours on %zu captured models, %zu state/view cases, %zu independently checked triangles (orange=%zu blue=%zu purple=%zu).\n",models,cases,triangles,orange,blue,purple);
}
