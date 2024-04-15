#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

constexpr float ModelMaxCoeff{494.38574219f};

class NNFilter final
{
public:
    NNFilter(std::string modelPath, int inputSize)
        : m_inputSize(inputSize)
    {
        m_model = cv::dnn::readNetFromONNX(modelPath);
        m_dataMat = cv::Mat(1, m_inputSize, CV_32F);
    }
    ~NNFilter() = default;

    float Infer(std::vector<float>& data)
    {
        m_dataMat.data = reinterpret_cast<unsigned char*>(data.data());
        cv::dnn::blobFromImage(m_dataMat, m_blob, 1.0 / ModelMaxCoeff, cv::Size() , cv::mean(m_dataMat));

        m_model.setInput(m_blob);
        m_softmaxProbs = m_model.forward();

        const float nonEventProb = m_softmaxProbs.at<float>(0);
        const float eventProb = 1.0f - nonEventProb;
        return eventProb;
    }

private:
    int m_inputSize{1};
    cv::dnn::Net m_model;
    cv::Mat m_dataMat;
    cv::Mat m_blob;
    cv::Mat m_softmaxProbs;
};
