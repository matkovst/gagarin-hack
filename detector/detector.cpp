#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "math.h"
#include "ffmpeg.h"

int main(int argc, char** argv)
{
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
    EventTrigger trigger(1, 8);

    uint8_t* pktData{nullptr};
    int pktSize = 0;
    int64_t pktPts = 0;
    int pktFlags = 0;
    uint64_t fno = 0;
    uint64_t IFrames = 0;
    int nEvents = 0;
    uint64_t firstKeyFrame = 0;
    do
    {
        demuxer.Demux(&pktData, &pktSize, &pktPts, &pktFlags);
        if (!isKeyFrame(pktFlags))
        {
            ++fno;
            continue;
        }

        // Способ 2. HACK.
        
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

        if (IFrames > static_cast<uint64_t>(n/2))
        {
            const int state = trigger.update( collector.outlier(kBytes, k) );
            if (EventTrigger::ABOUT_TO_ON == state)
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
        }

        collector.collect(kBytes);
        ++fno;
        ++IFrames;
    } while (pktSize);
    std::cout << "[Detector] Processed " << fno << " packets" << std::endl;
    std::cout << "[Detector] Processed " << IFrames << " I-frames" << std::endl;
    std::cout << "[Detector] Caught " << nEvents << " events" << std::endl;

	std::cout << "[Detector] Processing finished\n";
	return 0;
}
