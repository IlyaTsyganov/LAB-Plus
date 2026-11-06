#pragma once

// The engine tests the player's origin. Collider clearance is an extra preview
// margin, so its removal must not make origin-unique positions disappear.
// Paint origin-exclusive minus collider-cleared pieces amber; RNG overlap is
// absent from BOTH inputs and cannot enter this difference.
static SyncSectorMesh syncBuildClearanceBand(const SyncExclusiveGuide& origin,const SyncExclusiveGuide& body,const float* bounds) {
    SyncSectorMesh missing;if(!origin.known||!body.known)return missing;
    SyncExclusiveGuide band=origin;band.pieces.clear();band.boundary.clear();SyncExclusiveBudget budget;
    auto box=[](const SyncExclusivePolygon& p){std::array<double,4> b{DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX};for(auto v:p){b[0]=std::min(b[0],v.x);b[1]=std::min(b[1],v.y);b[2]=std::max(b[2],v.x);b[3]=std::max(b[3],v.y);}return b;};
    const double cell=1.;auto key=[](int x,int y){return (uint64_t(uint32_t(x))<<32)|uint32_t(y);};
    auto cells=[&](const std::array<double,4>& b){return std::array<int,4>{int(floor(b[0]/cell)),int(floor(b[1]/cell)),int(floor(b[2]/cell)),int(floor(b[3]/cell))};};
    std::unordered_map<uint64_t,std::vector<size_t>> grid;size_t indexing=0;
    for(size_t i=0;i<body.pieces.size();++i){const auto b=cells(box(body.pieces[i]));for(int x=b[0];x<=b[2];++x)for(int y=b[1];y<=b[3];++y){if(++indexing>100000)return missing;grid[key(x,y)].push_back(i);}}
    std::vector<size_t> seen(body.pieces.size(),size_t(-1));size_t stamp=0;
    for(const auto& source:origin.pieces){std::vector<SyncExclusivePolygon> remaining{source};const auto b=cells(box(source));++stamp;std::vector<size_t> candidates;
        for(int x=b[0];x<=b[2];++x)for(int y=b[1];y<=b[3];++y){if(++indexing>200000)return missing;const auto found=grid.find(key(x,y));if(found==grid.end())continue;
            for(auto i:found->second)if(seen[i]!=stamp){seen[i]=stamp;candidates.push_back(i);}}
        std::sort(candidates.begin(),candidates.end());
        for(auto i:candidates){std::vector<SyncExclusivePolygon> next;for(const auto& p:remaining){syncExclusiveSubtract(p,body.pieces[i],next,budget,4096);if(budget.exhausted)return missing;}remaining=std::move(next);if(remaining.empty())break;}
        for(auto& p:remaining)band.pieces.push_back(std::move(p));if(band.pieces.size()>4096)return missing;}
    // Only caps are needed here. The renderer uses the origin mesh's outer
    // walls and contour once, so there are no interior clearance walls or
    // expensive cancellation of the difference partition's internal edges.
    SyncSectorMesh result;if(!syncAimBoundsValid(bounds))return result;
    const SyncExclusivePolygon targetBox{{bounds[0],bounds[1]},{bounds[3],bounds[1]},{bounds[3],bounds[4]},{bounds[0],bounds[4]}};
    std::vector<SyncExclusivePolygon> caps;for(const auto& p:band.pieces){syncExclusiveSubtract(p,targetBox,caps,budget,4096);if(budget.exhausted||caps.size()>4096)return missing;}
    for(const auto& p:caps)for(size_t i=1;i+1<p.size();++i)for(float z:{bounds[2],bounds[5]})for(auto point:{p[0],p[i],p[i+1]})result.fill.push_back({float(point.x),float(point.y),z});
    result.known=true;return result;
}
static void appendSyncSectorFaces(const SyncSectorMesh& mesh,std::vector<Vertex>& output,SyncSectorTone tone,bool caps) {
    if(!mesh.known)return;const auto color=syncSectorColor(tone,false);
    for(size_t i=0;i+2<mesh.fill.size();i+=3){const bool cap=mesh.fill[i].z==mesh.fill[i+1].z&&mesh.fill[i].z==mesh.fill[i+2].z;if(cap!=caps)continue;
        for(unsigned j=0;j<3;++j){Vertex v{};v.position=mesh.fill[i+j];memcpy(v.color,color.data(),16);output.push_back(v);}}
}
