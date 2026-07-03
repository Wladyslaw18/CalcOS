#ifndef CALC_HPP
#define CALC_HPP

#include "calc.h"
#include <stdexcept>
#include <string>

namespace calc {

/**
 * @brief C++ RAII wrapper around the C calculator API.
 * Cache-line aligned (64-bytes), completely stack/inline allocated, 0 heap allocations.
 */
class alignas(64) Calculator {
public:
    /**
     * @brief Constructs a new Calculator object and initializes its internal state.
     */
    Calculator() {
        if (calc_state_size() > sizeof(m_inline_buffer)) {
            throw std::runtime_error("State structure exceeds inline buffer capacity");
        }
        m_state = reinterpret_cast<calc_state_t*>(m_inline_buffer);
        calc_state_init(m_state);
    }

    ~Calculator() = default;

    // Disable copy semantics to prevent state sharing and multiple free attempts
    Calculator(const Calculator&) = delete;
    Calculator& operator=(const Calculator&) = delete;

    // Move semantics must be custom to prevent internal pointers from pointing to the moved-from object's inline buffer!
    Calculator(Calculator&& other) noexcept {
        for (size_t i = 0; i < sizeof(m_inline_buffer); ++i) {
            m_inline_buffer[i] = other.m_inline_buffer[i];
        }
        m_state = reinterpret_cast<calc_state_t*>(m_inline_buffer);
        other.m_state = nullptr;
    }

    Calculator& operator=(Calculator&& other) noexcept {
        if (this != &other) {
            for (size_t i = 0; i < sizeof(m_inline_buffer); ++i) {
                m_inline_buffer[i] = other.m_inline_buffer[i];
            }
            m_state = reinterpret_cast<calc_state_t*>(m_inline_buffer);
            other.m_state = nullptr;
        }
        return *this;
    }

    /**
     * @brief Evaluates an expression. Throws std::runtime_error on failure.
     * 
     * @param expression The mathematical expression to evaluate.
     * @return The result of the evaluation.
     * @throws std::runtime_error if evaluation fails.
     */
    double evaluate(const std::string& expression) {
        bool success = false;
        double result = calc_evaluate(m_state, expression.c_str(), &success);
        if (!success) {
            throw std::runtime_error("Calculator expression evaluation failed");
        }
        return result;
    }

    /**
     * @brief Evaluates an expression, returning whether the operation succeeded in a reference.
     * 
     * @param expression The mathematical expression to evaluate.
     * @param success Reference to a boolean indicating success of the operation.
     * @return The result of the evaluation (0.0 on failure).
     */
    double evaluate(const std::string& expression, bool& success) noexcept {
        return calc_evaluate(m_state, expression.c_str(), &success);
    }

    /**
     * @brief Gets current calculator status/error flags.
     * 
     * @return Flags byte.
     */
    uint8_t get_flags() const noexcept {
        return calc_get_flags(m_state);
    }

    /**
     * @brief Clears current calculator status/error flags.
     */
    void clear_flags() noexcept {
        calc_clear_flags(m_state);
    }

    /**
     * @brief Gets the current mode.
     * 
     * @return Mode byte.
     */
    uint8_t get_mode() const noexcept {
        return calc_get_mode(m_state);
    }

    /**
     * @brief Sets the current mode.
     * 
     * @param mode The new mode.
     */
    void set_mode(uint8_t mode) noexcept {
        calc_set_mode(m_state, mode);
    }

private:
    alignas(64) uint8_t m_inline_buffer[64];
    calc_state_t* m_state{nullptr};
};

} // namespace calc

#endif // CALC_HPP
