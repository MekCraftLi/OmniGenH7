/**
 *******************************************************************************
 * @file    system_status.cpp
 * @brief   Unified system request factory implementations
 *******************************************************************************
 * @attention
 *
 * Factory helpers keep query request construction consistent across producers.
 *
 *******************************************************************************
 * @note
 *
 * Request sequence values are monotonically increasing within this process.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/05/17
 * @version 1.0
 *******************************************************************************
 */

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "domain/system_status.hpp"

namespace omnigen {

/*-------- 2. variables ----------------------------------------------------------------------------------------------*/

static uint32_t g_request_sequence = 0U;

/*-------- 3. implementation -----------------------------------------------------------------------------------------*/

AppRequest AppRequest::make(AppRequestKind kind)
{
    AppRequest request{};
    request.kind = kind;
    request.sequence = ++g_request_sequence;
    return request;
}

} // namespace omnigen
