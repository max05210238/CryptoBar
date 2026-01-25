// CryptoBar V0.99t - Non-blocking Network Task Implementation
// Background network operations using FreeRTOS task on Core 0

#include "network_task.h"
#include "app_state.h"
#include "network.h"
#include "coins.h"
#include <WiFi.h>

// ==================== Global State =====================

volatile NetworkTaskState g_netTaskState = NET_STATE_IDLE;
volatile bool g_netResultPending = false;
NetworkResult g_netResult;
SemaphoreHandle_t g_netMutex = nullptr;
QueueHandle_t g_netRequestQueue = nullptr;

// Coin loading state
volatile bool g_coinLoadingInProgress = false;
volatile bool g_historyLoadingInProgress = false;

// Internal state
static TaskHandle_t s_netTaskHandle = nullptr;
static uint32_t s_requestIdCounter = 0;

// Task configuration
static const int NET_TASK_STACK_SIZE = 8192;  // 8KB for HTTPClient
static const int NET_TASK_PRIORITY = 1;       // Low priority (same as LED task)
static const int NET_TASK_CORE = 0;           // Run on Core 0 (WiFi core)
static const int NET_QUEUE_SIZE = 4;          // Max pending requests

// ==================== Forward Declarations =====================

static void networkTaskMain(void* param);
static void processRequest(const NetworkRequest& req);
static void processFetchPrice(const NetworkRequest& req);
static void processBootstrapHistory(const NetworkRequest& req);
static void processFetchFxRates(const NetworkRequest& req);

// ==================== Public API Implementation =====================

void networkTaskInit() {
  Serial.println("[NetTask] Initializing network task...");

  // Create mutex for result protection
  g_netMutex = xSemaphoreCreateMutex();
  if (g_netMutex == nullptr) {
    Serial.println("[NetTask] ERROR: Failed to create mutex!");
    return;
  }

  // Create request queue
  g_netRequestQueue = xQueueCreate(NET_QUEUE_SIZE, sizeof(NetworkRequest));
  if (g_netRequestQueue == nullptr) {
    Serial.println("[NetTask] ERROR: Failed to create queue!");
    return;
  }

  // Initialize result
  memset(&g_netResult, 0, sizeof(g_netResult));

  // Create the network task pinned to Core 0
  BaseType_t result = xTaskCreatePinnedToCore(
    networkTaskMain,
    "netTask",
    NET_TASK_STACK_SIZE,
    nullptr,
    NET_TASK_PRIORITY,
    &s_netTaskHandle,
    NET_TASK_CORE
  );

  if (result == pdPASS) {
    Serial.printf("[NetTask] Task created on Core %d, stack=%d bytes\n",
                  NET_TASK_CORE, NET_TASK_STACK_SIZE);
  } else {
    Serial.println("[NetTask] ERROR: Failed to create task!");
  }
}

uint32_t networkTaskRequestPrice(int coinIndex) {
  if (g_netRequestQueue == nullptr) {
    Serial.println("[NetTask] Queue not initialized!");
    return 0;
  }

  NetworkRequest req;
  req.type = NET_REQ_FETCH_PRICE;
  req.coinIndex = coinIndex;
  req.requestId = ++s_requestIdCounter;

  // Clear any old pending result when starting new price fetch
  if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    g_netResultPending = false;
    xSemaphoreGive(g_netMutex);
  }

  // Set loading state
  g_coinLoadingInProgress = true;

  if (xQueueSend(g_netRequestQueue, &req, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.printf("[NetTask] Price request queued (id=%lu, coin=%d)\n",
                  (unsigned long)req.requestId, coinIndex);
    return req.requestId;
  } else {
    Serial.println("[NetTask] Queue full, request dropped!");
    g_coinLoadingInProgress = false;
    return 0;
  }
}

uint32_t networkTaskRequestHistory(int coinIndex) {
  if (g_netRequestQueue == nullptr) {
    Serial.println("[NetTask] Queue not initialized!");
    return 0;
  }

  NetworkRequest req;
  req.type = NET_REQ_BOOTSTRAP_HISTORY;
  req.coinIndex = coinIndex;
  req.requestId = ++s_requestIdCounter;

  // Set loading state
  g_historyLoadingInProgress = true;

  if (xQueueSend(g_netRequestQueue, &req, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.printf("[NetTask] History request queued (id=%lu, coin=%d)\n",
                  (unsigned long)req.requestId, coinIndex);
    return req.requestId;
  } else {
    Serial.println("[NetTask] Queue full, request dropped!");
    g_historyLoadingInProgress = false;
    return 0;
  }
}

uint32_t networkTaskRequestFxRates() {
  if (g_netRequestQueue == nullptr) {
    Serial.println("[NetTask] Queue not initialized!");
    return 0;
  }

  NetworkRequest req;
  req.type = NET_REQ_FETCH_FX_RATES;
  req.coinIndex = -1;
  req.requestId = ++s_requestIdCounter;

  if (xQueueSend(g_netRequestQueue, &req, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.printf("[NetTask] FX rates request queued (id=%lu)\n",
                  (unsigned long)req.requestId);
    return req.requestId;
  } else {
    Serial.println("[NetTask] Queue full, request dropped!");
    return 0;
  }
}

bool networkTaskHasResult() {
  bool pending = false;
  if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    pending = g_netResultPending;
    xSemaphoreGive(g_netMutex);
  }
  return pending;
}

NetworkResult networkTaskGetResult() {
  NetworkResult result;
  memset(&result, 0, sizeof(result));

  if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    result = g_netResult;
    xSemaphoreGive(g_netMutex);
  }

  return result;
}

void networkTaskConsumeResult() {
  if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    g_netResultPending = false;
    g_netTaskState = NET_STATE_IDLE;
    xSemaphoreGive(g_netMutex);
  }
}

bool networkTaskIsBusy() {
  return g_netTaskState == NET_STATE_BUSY;
}

void networkTaskCancelPending() {
  if (g_netRequestQueue == nullptr) return;

  // Clear the queue
  xQueueReset(g_netRequestQueue);

  // Clear loading states
  g_coinLoadingInProgress = false;
  g_historyLoadingInProgress = false;

  Serial.println("[NetTask] Pending requests cancelled");
}

// ==================== Task Implementation =====================

static void networkTaskMain(void* param) {
  Serial.println("[NetTask] Task started");

  NetworkRequest req;

  while (true) {
    // Wait for a request (blocking)
    if (xQueueReceive(g_netRequestQueue, &req, portMAX_DELAY) == pdTRUE) {
      // Update state to busy
      if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_netTaskState = NET_STATE_BUSY;
        xSemaphoreGive(g_netMutex);
      }

      Serial.printf("[NetTask] Processing request type=%d id=%lu\n",
                    req.type, (unsigned long)req.requestId);

      // Process the request
      processRequest(req);
    }
  }
}

static void processRequest(const NetworkRequest& req) {
  switch (req.type) {
    case NET_REQ_FETCH_PRICE:
      processFetchPrice(req);
      break;

    case NET_REQ_BOOTSTRAP_HISTORY:
      processBootstrapHistory(req);
      break;

    case NET_REQ_FETCH_FX_RATES:
      processFetchFxRates(req);
      break;

    default:
      Serial.printf("[NetTask] Unknown request type: %d\n", req.type);
      break;
  }
}

static void processFetchPrice(const NetworkRequest& req) {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NetTask] WiFi not connected, skipping price fetch");

    // Store failure result
    if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      g_netResult.type = NET_RESULT_PRICE;
      g_netResult.requestId = req.requestId;
      g_netResult.ok = false;
      g_netResult.price = 0.0;
      g_netResult.change24h = 0.0;
      g_netResultPending = true;
      g_netTaskState = NET_STATE_RESULT_READY;
      xSemaphoreGive(g_netMutex);
    }

    g_coinLoadingInProgress = false;
    return;
  }

  // Temporarily switch coin if specified
  int savedCoinIndex = g_currentCoinIndex;
  if (req.coinIndex >= 0 && req.coinIndex < coinCount()) {
    g_currentCoinIndex = req.coinIndex;
  }

  // Perform the fetch
  double price = 0.0;
  double change = 0.0;
  bool ok = fetchPrice(price, change);

  // Restore coin index if we changed it
  if (req.coinIndex >= 0) {
    g_currentCoinIndex = savedCoinIndex;
  }

  // Store result
  if (xSemaphoreTake(g_netMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    g_netResult.type = NET_RESULT_PRICE;
    g_netResult.requestId = req.requestId;
    g_netResult.ok = ok;
    g_netResult.price = price;
    g_netResult.change24h = change;
    g_netResultPending = true;
    g_netTaskState = NET_STATE_RESULT_READY;
    xSemaphoreGive(g_netMutex);
  }

  g_coinLoadingInProgress = false;

  Serial.printf("[NetTask] Price fetch %s: %.2f (%.2f%%)\n",
                ok ? "OK" : "FAILED", price, change);
}

static void processBootstrapHistory(const NetworkRequest& req) {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NetTask] WiFi not connected, skipping history fetch");
    g_historyLoadingInProgress = false;
    return;
  }

  // Temporarily switch coin if specified
  int savedCoinIndex = g_currentCoinIndex;
  if (req.coinIndex >= 0 && req.coinIndex < coinCount()) {
    g_currentCoinIndex = req.coinIndex;
  }

  // Reset chart before fetching
  g_chartSampleCount = 0;
  g_cycleInit = false;

  // Perform the fetch (writes directly to g_chartSamples)
  bootstrapHistoryFromKrakenOHLC();

  // Restore coin index if we changed it
  if (req.coinIndex >= 0) {
    g_currentCoinIndex = savedCoinIndex;
  }

  g_historyLoadingInProgress = false;

  Serial.printf("[NetTask] History fetch complete, samples=%d\n", g_chartSampleCount);
}

static void processFetchFxRates(const NetworkRequest& req) {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NetTask] WiFi not connected, skipping FX fetch");
    return;
  }

  // Perform the fetch (writes directly to g_usdToRate)
  fetchExchangeRates();

  Serial.println("[NetTask] FX rates fetch complete");
}
