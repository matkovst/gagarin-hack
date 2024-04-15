#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <deque>

#include "math.h"
#include "ffmpeg.h"
#include "nn_filter.h"

constexpr int OutlierTriggerInertia{10};
constexpr int PktQueueCapacity{2*OutlierTriggerInertia};

int main(int argc, char** argv)
{
    // Прочитать аргументы

    if (argc < 2)
    {
        std::cerr << "You must specify SOURCE\n";
        return 0;
    }

    int n = 12;
    if (argc > 2)
        n = atoi(argv[2]);
    std::cout << "[Detector] N = " << n << std::endl;

    float k = 3.0f;
    if (argc > 3)
        k = atof(argv[3]);
    std::cout << "[Detector] K = " << k << std::endl;

    float gradThr = 1000.0f / 1024.0f;
    if (argc > 4)
        k = atof(argv[4]);
    std::cout << "[Detector] T_grad = " << gradThr << std::endl;

    bool applyNn = false;
    if (argc > 5)
        applyNn = atoi(argv[5]);
    std::cout << "[Detector] Apply NN filter = " << std::boolalpha << applyNn << std::endl;

    // Подгрузить нейросетевую модель
    // TODO

    // HACK
    // int gopSize = 12;
    // if (argc > 4)
    //     gopSize = atoi(argv[4]);
    // std::cout << "GOP = " << gopSize << std::endl;

    std::string source(argv[1]);
    std::cout << "[Detector] Start processing \"" << source.c_str() << "\"" << std::endl;

    auto kB = [](int B) { return static_cast<float>(B) / 1024.0f; };
    auto isKeyFrame = [](int flags) { return flags & AV_PKT_FLAG_KEY; };

    FFmpegH264Demuxer demuxer(source.c_str());
    StatsCollector collector(n);
    HoldoutTrigger holdout(OutlierTriggerInertia-1);
    HoldoutTrigger nnFilterTrigger(OutlierTriggerInertia-1); // вспомогательный триггер, пока не сделаем нейрофильтр устойчивым к сдвигу сигнала
    NNFilter nnFilter("nn_filter.onnx", PktQueueCapacity);

    uint8_t* pktData{nullptr};
    int pktSize = 0;
    int64_t pktPts = 0;
    std::deque<float> pktSizeQueue;
    std::vector<float> slice(PktQueueCapacity, 0.0f);
    int pktFlags = 0;
    uint64_t fno = 0;
    uint64_t IFrames = 0;
    int nEvents = 0;
    uint64_t firstKeyFrame = 0;
    std::chrono::microseconds demuxerTook{0};
    std::chrono::microseconds collectorTook{0};
    do
    {
        using namespace std::chrono;

        // ---------- Perf ----------
        auto demuxStart = high_resolution_clock::now();
        // ---------- ---- ----------

        demuxer.Demux(&pktData, &pktSize, &pktPts, &pktFlags);

        // ---------- Perf ----------
        demuxerTook += duration_cast<microseconds>(high_resolution_clock::now() - demuxStart);
        // ---------- ---- ----------

        if (!isKeyFrame(pktFlags))
        {
            ++fno;
            continue;
        }

        // Способ 2. HACK для предсказания I-frame'ов (обычно их больше, чем ключевых пакетов).
        
        // if (0 == firstKeyFrame)
        // {
        //     if (isKeyFrame(pktFlags))
        //     {
        //         firstKeyFrame = fno;
        //     }
        //     else
        //     {
        //         ++fno;
        //         continue;
        //     }
        // }
        // bool isKeyFrame2 = ((fno - firstKeyFrame) % gopSize == 0);
        // if (!isKeyFrame2)
        // {
        //     ++fno;
        //     continue;
        // }

        const auto kBytes = kB(pktSize);

        if (pktSizeQueue.size() >= PktQueueCapacity)
            pktSizeQueue.pop_front();
        pktSizeQueue.push_back(kBytes);

        if (IFrames > static_cast<uint64_t>(n/2))
        {
            // Первичная классификация - принять решение
            const bool isOutlier = collector.outlier(kBytes, k);
            const bool highGradient = std::abs(collector.grad()) > gradThr;
            const bool isCandidate = (isOutlier && highGradient);

            const int state = holdout.update(isCandidate); // добавить "вязкость"
            const int state2 = nnFilterTrigger.update(isCandidate);

            // Событие!
            if (HoldoutTrigger::Up == state)
            {
                ++nEvents;
                int min = floor(pktPts / 1000.0 / 60.0);
                int sec = floor(pktPts / 1000.0) - 60.0*min;
                std::cout << "[Detector] EVENT! Outlier at " 
                          << pktPts 
                          << " | frame #" << IFrames 
                          << " | " << min << ":" << sec
                          << std::endl;
            }

            if (HoldoutTrigger::Down == state2)
            {
                if (applyNn)
                {
                    if (pktSizeQueue.size() >= PktQueueCapacity)
                    {
                        std::copy(pktSizeQueue.cbegin(), pktSizeQueue.cend(), slice.begin());
                        const float anomalyProb = nnFilter.Infer(slice);
                        // NOTE: модель неустойчива к сдвигу сигнала, поэтому решение нейрофильтра не всегда адекватное
                        std::cout << "[Detector] Anomaly prob for prev event: " 
                                  << anomalyProb << std::endl;
                    }
                    else
                    {
                        std::cout << "Too few packets received. NN filtering skipped.\n";
                    }
                }
            }
        }

        // ---------- Perf ----------
        auto collectorStart = high_resolution_clock::now();
        // ---------- ---- ----------

        collector.collect(kBytes);

        // ---------- Perf ----------
        collectorTook += duration_cast<microseconds>(high_resolution_clock::now() - collectorStart);
        // ---------- ---- ----------

        ++fno;
        ++IFrames;
    } while (pktSize);

    std::cout << "[Detector] Processed " << fno << " packets" << std::endl;
    std::cout << "[Detector] Processed " << IFrames << " I-frames" << std::endl;
    std::cout << "[Detector] Caught " << nEvents << " events" << std::endl;
    if (fno > 0)
        std::cout << "[Detector] Demuxer took " << (demuxerTook.count() / fno) << " μs on average" << std::endl;
    if (IFrames > 0)
        std::cout << "[Detector] Collector took " << (collectorTook.count() / IFrames) << " μs on average" << std::endl;

    std::cout << "[Detector] Processing finished\n";
    return 0;
}
