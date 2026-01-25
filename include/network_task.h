// CryptoBar V0.99t - Non-blocking Network Task
// Background network operations to keep UI responsive
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// ==================== Request Types =====================

enum NetworkRequestType {
  NET_REQ_NONE = 0,
  NET_REQ_FETCH_PRICE,           // Fetch current price for a coin
  NET_REQ_BOOTSTRAP_HISTORY,     // Fetch OHLC history for chart
  NET_REQ_FETCH_FX_RATES,        // Fetch currency exchange rates
};

struct NetworkRequest {
  NetworkRequestType type;
  int coinIndex;                 // Which coin (-1 = use g_currentCoinIndex)
  uint32_t requestId;            // Unique ID for result matching
};

// ==================== Result Types =====================

enum NetworkResultType {
  NET_RESULT_NONE = 0,
  NET_RESULT_PRICE,
  NET_RESULT_HISTORY,
  NET_RESULT_FX_RATES,
};

struct NetworkResult {
  NetworkResultType type;
  uint32_t requestId;
  bool ok;                       // Success or failure

  // Price result
  double price;
  double change24h;

  // History result (stored directly to g_chartSamples)
  // FX result (stored directly to g_usdToRate)
};

// ==================== Task State =====================

enum NetworkTaskState {
  NET_STATE_IDLE = 0,
  NET_STATE_BUSY,
  NET_STATE_RESULT_READY,
};

// ==================== Global State (extern) =====================

extern volatile NetworkTaskState g_netTaskState;
extern volatile bool g_netResultPending;
extern NetworkResult g_netResult;
extern SemaphoreHandle_t g_netMutex;
extern QueueHandle_t g_netRequestQueue;

// Coin loading state (for UI)
extern volatile bool g_coinLoadingInProgress;
extern volatile bool g_historyLoadingInProgress;

// ==================== Public API =====================

// Initialize the network task (call once in setup())
void networkTaskInit();

// Request functions (non-blocking, returns immediately)
// Returns request ID for tracking
uint32_t networkTaskRequestPrice(int coinIndex = -1);
uint32_t networkTaskRequestHistory(int coinIndex = -1);
uint32_t networkTaskRequestFxRates();

// Check if a result is ready
bool networkTaskHasResult();

// Get the result (call only when hasResult() returns true)
// Returns a copy of the result
NetworkResult networkTaskGetResult();

// Consume the result (mark as processed)
void networkTaskConsumeResult();

// Check if task is currently busy
bool networkTaskIsBusy();

// Cancel pending requests (e.g., when switching coins rapidly)
void networkTaskCancelPending();
