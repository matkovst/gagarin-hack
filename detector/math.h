#include <cmath>

class StatsCollector final
{
public:
    StatsCollector(int n)
    {
        if (n > 0)
        {
            m_forgetting = 2.0f / (n + 1);
            m_forgettingInv = 1.0f - m_forgetting;
        }
    }
    ~StatsCollector() = default;

    void collect(float x)
    {
        if (m_first)
        {
            m_mean = x;
            m_x0 = x;
            m_first = false;
            return;
        }

        const float d = x - m_mean;
        m_mean = m_mean + (m_forgetting * d);
        m_stdev2 = m_stdev2 * m_forgettingInv + m_forgetting * d * d;
        m_grad = x - m_x0;
        m_x0 = x;
    }

    bool outlier(float x, float k)
    {
        const float d = x - m_mean;
        return static_cast<bool>((d*d) > (k*k*m_stdev2));
    }

    float mean() const noexcept { return m_mean; }
    float stdev2() const noexcept { return m_stdev2; }
    float grad() const noexcept { return m_grad; }

private:
    bool m_first{true};
    float m_forgetting{1.0};
    float m_forgettingInv{0.0};
    float m_mean{0.0f};
    float m_stdev2{1.0f};
    float m_grad{0.0f};
    float m_x0{0.0f};
};

class HoldoutTrigger final
{
public:
    enum Result { Up, Down, Hold };

    HoldoutTrigger(int countBeforeDown = 1)
    {
        m_countBeforeDown = (countBeforeDown > 0) ? countBeforeDown : 1;
    }
    ~HoldoutTrigger() = default;

    int update(bool v)
    {
        if (m_up)
        {
            ++m_upCount;
            if (m_upCount > m_countBeforeDown)
            {
                m_up = false;
                return Down;
            }
        }
        else
        {
            if (v)
            {
                m_up = true;
                m_upCount = 0;
                return Up;
            }
        }

        return Hold;
    }

private:
    int m_countBeforeDown{1};
    bool m_up{false};
    int m_upCount{0};
};

class EventTrigger final
{
public:

    enum State
    {
        ABOUT_TO_OFF    = -1,
        OFF             = 0,
        ABOUT_TO_ON     = 1,
        ON              = 2
    };

    EventTrigger(int countBeforeOn = 1, int countBeforeOff = 1)
    {
        m_countBeforeOn = (countBeforeOn > 0) ? countBeforeOn : 1;
        m_countBeforeOff = (countBeforeOff > 0) ? countBeforeOff : 1;
    }

    ~EventTrigger() = default;

    void init(int countBeforeOn = 1, int countBeforeOff = 1)
    {
        m_countBeforeOn = (countBeforeOn > 0) ? countBeforeOn : 1;
        m_countBeforeOff = (countBeforeOff > 0) ? countBeforeOff : 1;
        m_counter = 0;
        m_state = false;
    }

    int update(bool v)
    {
        bool state = count(v);
        int triggerState = State::OFF;
        if ( state )
        {
            if ( m_state )
            {
                triggerState = State::ON;
            }
            else
            {
                triggerState = State::ABOUT_TO_ON;
                m_counter += m_countBeforeOff;
            }
        }
        else
        {
            if ( m_state )
            {
                triggerState = State::ABOUT_TO_OFF;
            }
            else
            {
                triggerState = State::OFF;
            }
        }
        m_state = state;
        return triggerState;
    }

    int state() const noexcept
    {
        return m_state;
    }

    int operator()(bool i)
    {
        return update(i);
    }

private:
    int m_countBeforeOn { 1 };
    int m_countBeforeOff { 1 };
    int m_counter { 0 };
    bool m_state { false };

    bool count(bool v)
    {
        m_counter += v ? v : -1;
        if (m_counter < 0) m_counter = 0;
        if (m_counter > m_countBeforeOn + m_countBeforeOff) m_counter = (m_countBeforeOn + m_countBeforeOff);
        return (m_counter >= m_countBeforeOn);
    }

};
