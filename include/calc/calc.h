#ifndef CALC_H
#define CALC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing the internal state of the calculator.
 */
typedef struct calc_state calc_state_t;

/**
 * @brief Returns the size in bytes of the calculator state structure.
 * 
 * @return Size of the state in bytes.
 */
size_t calc_state_size(void);

/**
 * @brief Initializes the calculator state.
 * 
 * @param state Pointer to the memory block allocated for the calculator state.
 */
void calc_state_init(calc_state_t* state);

/**
 * @brief Evaluates an expression using the calculator state.
 * 
 * @param state Pointer to the initialized calculator state.
 * @param expression Null-terminated string containing the mathematical expression.
 * @param success Pointer to a boolean that will be set to true on success, or false on failure.
 * @return The result of the evaluation as a double.
 */
double calc_evaluate(calc_state_t* state, const char* expression, bool* success);

/**
 * @brief Gets the current status/error flags of the calculator.
 * 
 * @param state Pointer to the calculator state.
 * @return The current flags byte.
 */
uint8_t calc_get_flags(const calc_state_t* state);

/**
 * @brief Clears the status/error flags of the calculator.
 * 
 * @param state Pointer to the calculator state.
 */
void calc_clear_flags(calc_state_t* state);

/**
 * @brief Gets the current mode of the calculator.
 * 
 * @param state Pointer to the calculator state.
 * @return The current mode byte.
 */
uint8_t calc_get_mode(const calc_state_t* state);

/**
 * @brief Sets the current mode of the calculator.
 * 
 * @param state Pointer to the calculator state.
 * @param mode The new mode byte.
 */
void calc_set_mode(calc_state_t* state, uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif // CALC_H
