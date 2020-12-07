/**
 * @file errors.h
 * @ingroup service
 * @brief Error cases 
 *
 * @anchor table_of_error_cases 
 * @verbatim
 * ---------------+-------------------------------+--------------------------
 * error case     | Meaning                       | simulation goal
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT1   = 1 | DAT packet with sequence      | failing connection
 *                | number 1 is dropped           | establishment
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT2   = 2 | DAT packet with sequence      | faulty transmission
 *                | number 2 is dropped           | -> Go-Back-N  
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT4   = 3 | 1st DAT packet with sequence  | faulty transmission
 *                | number 4 is dropped           | -> Go-Back-N  
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT3UP = 4 | DAT packets with sequence     | faulty transmission 
 *                | number > 2 are dropped        | -> connection abort
 * ---------------+-------------------------------+--------------------------
 * ERR_ACK1   = 5 | ACK packet with sequence      | failing connection
 *                | number 1 is dropped           | establishment 
 * ---------------+-------------------------------+-------------------------- 
 * ERR_ACK3   = 6 | ACK packet with sequence      | faulty transmission  
 *                | number 3 is dropped           | -> no direct impact                 
 * ---------------+-------------------------------+-------------------------- 
 * ERR_ACK4UP = 7 | ACK packets with sequence     | faulty transmision
 *                | number > 3 are dropped        | -> connection abort
 * ---------------+-------------------------------+--------------------------
 * ERR_ABO    = 8 | ACK packets with sequence     | faulty transmision 
 *                | number > 3 and all            | -> connection abort
 *                | ABO packets are dropped       |     
 * ---------------+-------------------------------+--------------------------
 *
 * ---------------+-------------------------------+--------------------------
 * Fehlerfall     | Bedeutung                     | Simulationsziel
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT1   = 1 | Das DAT-Paket mit der         | fehlerhafter Verbindungs-
 *                | Sequenznummer 1 wird          | aufbau
 *                | verworfen.                    |
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT2   = 2 | Das DAT-Paket mit der         | fehlerhafte Uebertragung
 *                | Sequenznummer 2 wird          | -> Go-Back-N  
 *                | verworfen.                    |
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT4   = 3 | Das erste DAT-Paket mit der   | fehlerhafte Uebertragung
 *                | Sequenznummer 4 wird          | -> Go-Back-N  
 *                | verworfen.                    |
 * ---------------+-------------------------------+--------------------------
 * ERR_DAT3UP = 4 | Die DAT-Pakete mit einer      | fehlerhafte Uebertragung 
 *                | Sequenznummer > 2 werden      | -> Abbruch der Verbindung
 *                | verworfen.                    |  
 * ---------------+-------------------------------+--------------------------
 * ERR_ACK1   = 5 | Das ACK-Paket mit der         | fehlerhafter Verbindungs-
 *                | Sequenznummer 1 wird          | aufbau 
 *                | verworfen.                    |  
 * ---------------+-------------------------------+-------------------------- 
 * ERR_ACK3   = 6 | Das ACK-Paket mit der         | fehlerhafte Verbindung   
 *                | Sequenznummer 3 wird          | -> keine direkte Auswir- 
 *                | verworfen.                    | ung                    
 * ---------------+-------------------------------+-------------------------- 
 * ERR_ACK4UP = 7 | Die ACK-Pakete mit einer      | fehlerhafte Verbindung 
 *                | Sequenznummer > 3 werden      | -> Verbindungsabbruch 
 *                | verworfen.                    | 
 * ---------------+-------------------------------+--------------------------
 * ERR_ABO    = 8 | Die ACK-Pakete mit einer      | fehlerhafte Verbindung 
 *                | Sequenznummer > 3 und alle    | -> Verbindungsabbruch 
 *                | ABO-Pakete werden verworfen.  |     
 * ---------------+-------------------------------+--------------------------
 * @endverbatim
 */

#ifndef ERRORS_H
#define ERRORS_H


/**
 * @addtogroup service
 * @{
 */


#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>


/** @brief The several error cases */
typedef enum
{
  ERR_NO = 0, /**< no error case (only for convenient) */
  ERR_DAT1, /**< see @ref table_of_error_cases "Table" */
  ERR_DAT2, /**< see @ref table_of_error_cases "Table" */
  ERR_DAT4, /**< see @ref table_of_error_cases "Table" */
  ERR_DAT3UP, /**< see @ref table_of_error_cases "Table" */
  ERR_ACK1, /**< see @ref table_of_error_cases "Table" */
  ERR_ACK3, /**< see @ref table_of_error_cases "Table" */
  ERR_ACK4UP, /**< see @ref table_of_error_cases "Table" */
  ERR_ABO, /**< see @ref table_of_error_cases "Table" */
  ERR_MAX_SUCC /**< invalid error case (only for convenient) */
} XDT_error;


ssize_t send_err(int s, void *msg, size_t len, XDT_error error_case);
ssize_t sendto_err(int s, void *msg, size_t len, XDT_error error_case, const struct sockaddr *to, socklen_t tolen);

/**
 * @}
 */

#endif /* ERRORS_H */
