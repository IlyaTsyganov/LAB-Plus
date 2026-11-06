#pragma once
#include <list>

// Physics world snapshots are immutable. Reusing their transformed positions
// avoids repeating every affine transform on the presentation thread. Colors,
// selection, opacity and live entities are still evaluated for the current frame.
class WorldVertexCache {
    struct Entry { std::vector<Vec3> positions; std::list<const Mesh*>::iterator age; };
    std::unordered_map<const Mesh*,Entry> entries;
    std::list<const Mesh*> recent;
    std::weak_ptr<const RuntimeScene> owner;
    std::weak_ptr<const ProjectileTrim> trimOwner;
    const Mesh* first=nullptr;
    const Mesh* last=nullptr;
    const Mesh* trimFirst=nullptr;
    const Mesh* trimLast=nullptr;
    size_t used=0;
    bool active=false;
public:
    static constexpr size_t limit=32*1024*1024;
    void begin(const std::shared_ptr<const RuntimeScene>& scene,const std::vector<Candidate>& visible,const std::shared_ptr<const ProjectileTrim>& trim={}) {
        if(!scene||owner.owner_before(scene)||scene.owner_before(owner)) {
            entries.clear(); recent.clear(); used=0;
        } else if(trimOwner.owner_before(trim)||trim.owner_before(trimOwner)) {
            // A new clipping batch leaves the original world immutable. Keep
            // its warm positions and evict only replaced projectile instances.
            const std::less<const Mesh*> before;
            for(auto it=entries.begin();it!=entries.end();) {
                if(first&&!before(it->first,first)&&before(it->first,last)) { ++it; continue; }
                used-=it->second.positions.size()*sizeof(Vec3); recent.erase(it->second.age); it=entries.erase(it);
            }
        }
        owner=scene; trimOwner=trim;
        first=scene&&!scene->meshes.empty()?scene->meshes.data():nullptr;
        last=first?first+scene->meshes.size():nullptr;
        trimFirst=scene&&trim&&!trim->meshes.empty()?trim->meshes.data():nullptr;
        trimLast=trimFirst?trimFirst+trim->meshes.size():nullptr;
        active=first!=nullptr;
        // A visible set larger than the cache would evict itself twice per frame
        // (fill then lines). Keep the normal transform path for such large views.
        size_t visibleBytes=0;
        if(active) for(const auto& candidate:visible) if(contains(candidate.mesh)) {
            const auto& geometry=*candidate.mesh->geometry;
            const size_t count=geometry.triangles.size()+geometry.lines.size();
            if(count>limit/sizeof(Vec3)||visibleBytes>limit-count*sizeof(Vec3)) { active=false; break; }
            visibleBytes+=count*sizeof(Vec3);
        }
    }
    bool contains(const Mesh* mesh) const {
        const std::less<const Mesh*> before;
        return (first&&!before(mesh,first)&&before(mesh,last))||(trimFirst&&!before(mesh,trimFirst)&&before(mesh,trimLast));
    }
    const std::vector<Vec3>* positions(const Mesh& mesh) {
        if(!active||!contains(&mesh)) return nullptr;
        auto found=entries.find(&mesh);
        if(found!=entries.end()) {
            recent.splice(recent.end(),recent,found->second.age);
            return &found->second.positions;
        }
        const auto& geometry=*mesh.geometry;
        const size_t count=geometry.triangles.size()+geometry.lines.size(),bytes=count*sizeof(Vec3);
        if(bytes>limit) return nullptr;
        while(used>limit-bytes&&!recent.empty()) {
            auto old=entries.find(recent.front());
            used-=old->second.positions.size()*sizeof(Vec3); entries.erase(old); recent.pop_front();
        }
        Entry entry;
        entry.positions.reserve(count);
        for(Vec3 point:geometry.triangles) entry.positions.push_back(mesh.world(point));
        for(Vec3 point:geometry.lines) entry.positions.push_back(mesh.world(point));
        recent.push_back(&mesh); entry.age=std::prev(recent.end()); used+=bytes;
        return &entries.emplace(&mesh,std::move(entry)).first->second.positions;
    }
};
