#pragma once
#include <mmsystem.h>

// Two low-pass stages retain bass while reducing mids and treble more
// strongly than the previous single stage. State belongs to each channel.
static float funMuffledSample(float sample,float& first,float& second) {
    first+=.05f*(sample-first);
    second+=.05f*(first-second);
    return second;
}

// A private waveOut stream: all gain/filtering happens in our PCM samples.
// Never change the Windows device/session volume or the game's SFX/VO buses.
class FunAudio {
    HMODULE library=nullptr;
    HWAVEOUT device=nullptr;
    decltype(&waveOutOpen) open=nullptr;
    decltype(&waveOutPrepareHeader) prepare=nullptr;
    decltype(&waveOutWrite) write=nullptr;
    decltype(&waveOutReset) reset=nullptr;
    decltype(&waveOutUnprepareHeader) unprepare=nullptr;
    decltype(&waveOutClose) close=nullptr;
    std::vector<int16_t> pcm;
    std::thread worker;
    std::atomic<bool> stopping{false},failed{false};
    std::atomic<float> gain{0},muffle{0},pan{0};
    std::atomic<uint64_t> played{0};
    uint32_t rate=48000;
public:
    ~FunAudio() { stop(); if(library) FreeLibrary(library); }
    bool start(const std::filesystem::path& path) {
        stop();
        std::ifstream input(path,std::ios::binary);
        char riff[12]{}; if(!input.read(riff,12)||memcmp(riff,"RIFF",4)||memcmp(riff+8,"WAVE",4)) return false;
        WAVEFORMATEX format{}; bool valid=false,found=false;
        for(unsigned chunk=0;chunk<128&&input;++chunk) {
            char name[4]; uint32_t size=0;
            if(!input.read(name,4)||!input.read(reinterpret_cast<char*>(&size),4)||size>128*1024*1024) break;
            if(!memcmp(name,"fmt ",4)&&size>=16&&size<=1024) {
                if(!input.read(reinterpret_cast<char*>(&format),16)) return false;
                valid=format.wFormatTag==WAVE_FORMAT_PCM&&format.nChannels==2&&format.wBitsPerSample==16&&
                    format.nSamplesPerSec==48000&&format.nBlockAlign==4&&format.nAvgBytesPerSec==192000;
                input.seekg(size-16,std::ios::cur);
            } else if(!memcmp(name,"data",4)&&size>=4&&size%4==0) {
                pcm.resize(size/2); if(!input.read(reinterpret_cast<char*>(pcm.data()),size)) return false; found=true;
            } else input.seekg(size,std::ios::cur);
            if(size&1) input.seekg(1,std::ios::cur);
            if(valid&&found) break;
        }
        if(!valid||!found) { pcm.clear(); return false; }
        if(!library) library=LoadLibraryW(L"winmm.dll"); if(!library) return false;
#define FUN_AUDIO_PROC(member,name) member=reinterpret_cast<decltype(member)>(GetProcAddress(library,name)); if(!member) return false
        FUN_AUDIO_PROC(open,"waveOutOpen"); FUN_AUDIO_PROC(prepare,"waveOutPrepareHeader"); FUN_AUDIO_PROC(write,"waveOutWrite");
        FUN_AUDIO_PROC(reset,"waveOutReset"); FUN_AUDIO_PROC(unprepare,"waveOutUnprepareHeader"); FUN_AUDIO_PROC(close,"waveOutClose");
#undef FUN_AUDIO_PROC
        if(open(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) { device=nullptr; return false; }
        rate=format.nSamplesPerSec; stopping=false; failed=false; gain=0; muffle=0; pan=0; played=0;
        try { worker=std::thread([this] { run(); }); }
        catch(...) { close(device); device=nullptr; return false; }
        return true;
    }
    void mix(float volume,bool helmetOutside,float position=0) { gain=std::clamp(volume,0.f,1.f); muffle=helmetOutside?1.f:0.f; pan=std::clamp(position,-.95f,.95f); }
    bool healthy() const { return !failed.load(); }
    double seconds() const { return double(played.load())/rate; }
    void stop() {
        stopping=true;
        if(worker.joinable()) worker.join();
        if(device) { reset(device); close(device); device=nullptr; }
        pcm.clear(); gain=0; played=0;
    }
private:
    void run() {
        SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
        // 128 ms of queued PCM tolerates short game/notification CPU spikes.
        constexpr size_t frames=768,count=8;
        std::array<std::array<int16_t,frames*2>,count> buffers{};
        std::array<WAVEHDR,count> headers{};
        size_t prepared=0,cursor=0; uint64_t queued=0; float volume=0,low[2]{},lowSecond[2]{},filterMix=0,stereoPan=0;
        ULONGLONG presenceAt=GetTickCount64()+100;
        for(size_t i=0;i<count;++i) {
            headers[i].lpData=reinterpret_cast<char*>(buffers[i].data()); headers[i].dwBufferLength=frames*4;
            if(prepare(device,&headers[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR) { failed=true; break; }
            ++prepared;
        }
        while(!stopping.load()&&!failed.load()) {
            const auto now=GetTickCount64();
            if(now>=presenceAt) { presenceAt=now+100; if(!labAppAvailable()) { failed=true; break; } }
            bool submitted=false;
            for(size_t i=0;i<prepared&&!stopping.load();++i) {
                auto& h=headers[i]; if((h.dwFlags&WHDR_INQUEUE)&&!(h.dwFlags&WHDR_DONE)) continue;
                if(h.dwFlags&WHDR_DONE) played.fetch_add(frames);
                const float target=gain.load(),targetFilter=muffle.load(),targetPan=pan.load();
                for(size_t f=0;f<frames;++f) {
                    // 60 ms gain ramp and 100 ms low-pass blend prevent clicks
                    // during a first/third person camera cut or a menu pause.
                    volume+=(target-volume)*.00035f; filterMix+=(targetFilter-filterMix)*.00021f; stereoPan+=(targetPan-stereoPan)*.00021f;
                    for(size_t ch=0;ch<2;++ch) {
                        float sample=pcm[cursor+ch];
                        float muffled=funMuffledSample(sample,low[ch],lowSecond[ch]);
                        float filtered=sample+(muffled-sample)*filterMix;
                        filtered*=volume*(1.f-.72f*filterMix);
                        filtered*=ch==0?sqrtf(1.f-std::max(0.f,stereoPan)):sqrtf(1.f+std::min(0.f,stereoPan));
                        buffers[i][f*2+ch]=static_cast<int16_t>(std::clamp(filtered,-32768.f,32767.f));
                    }
                    cursor=(cursor+2)%pcm.size();
                }
                if(write(device,&h,sizeof(h))!=MMSYSERR_NOERROR) { failed=true; break; }
                queued+=frames; submitted=true;
            }
            if(!submitted) std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        reset(device);
        for(size_t i=0;i<prepared;++i) unprepare(device,&headers[i],sizeof(WAVEHDR));
    }
};
