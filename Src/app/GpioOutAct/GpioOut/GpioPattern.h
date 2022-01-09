#ifndef GPIO_PATTERN_H
#define GPIO_PATTERN_H

#include "bsp.h"
#include <stdint.h>
#include "fw_log.h"
#include "qassert.h"

#define GPIO_PATTERN_ASSERT(t_) ((t_)? (void)0: Q_onAssert("GpioPattern.h", (int32_t)__LINE__))

namespace APP {

class GpioInterval {
public:
    uint16_t m_levelPermil;     // Brightness level 0-1000.
    uint16_t m_durationMs;      // Duration in millisecond.
    
    uint16_t GetLevelPermil() const { return m_levelPermil; }
    uint16_t GetDurationMs() const { return m_durationMs; }
};

class GpioPattern {
public:
    enum {
        COUNT = 256
    };
    uint32_t m_count;                // Number of intervals in use.
    GpioInterval m_interval[COUNT];   // Array of intervals. Used ones start from index 0.
    
    // Must perform range check. Assert if invalid.
    uint32_t GetCount() const { 
        return m_count;
    }
    GpioInterval const &GetInterval(uint32_t index) const {
        GPIO_PATTERN_ASSERT(index < m_count);
        return m_interval[index];
    }
};

class GpioPatternSet {
public:
    enum {
        COUNT = 4
    };
    uint32_t m_count;               // Number of patterns in use.
    GpioPattern m_pattern[COUNT];    // Array of patterns. Used ones start from index 0.
    
    // Must perform range check. Assert if invalid.
    uint32_t GetCount() const {
        return m_count;
    }
    GpioPattern const *GetPattern(uint32_t index) const {
        if (index < m_count) {
            return &m_pattern[index];
        }
        return NULL;
    }
};

extern GpioPatternSet const TEST_GPIO_PATTERN_SET;

} // namespace APP

#endif // GPIO_PATTERN_H
