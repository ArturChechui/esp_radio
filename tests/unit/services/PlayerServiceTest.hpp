#pragma once

#include <gtest/gtest.h>

// Common
#include "FakeAudioBufferStats.hpp"
#include "FakeRingBuffer.hpp"
#include "MockEventQueue.hpp"
#include "MockStopToken.hpp"
#include "MockTaskRunner.hpp"

// Adapters
#include "MockHttpClient.hpp"
#include "MockI2sBus.hpp"
#include "MockMp3Decoder.hpp"

// Service
#include "PlayerService.hpp"
#include "Types.hpp"

class PlayerServiceTest : public ::testing::Test {
   protected:
    void SetUp() override;
    void TearDown() override;

    void expectStartCaptureBothStepFns();
    void configureNonStoppingToken(common::MockStopToken& token);

    std::unique_ptr<adapters::MockHttpClient> mockHttpClient;
    std::unique_ptr<adapters::MockI2sBus> mockI2sBus;
    std::unique_ptr<adapters::MockMp3Decoder> mockMp3Decoder;
    std::unique_ptr<common::FakeAudioBufferStats> fakeStats;
    std::unique_ptr<common::MockEventQueue> mockEventQueue;
    std::unique_ptr<common::MockTaskRunner> mockTaskRunner;

    common::FakeRingBuffer* fakeRing = nullptr;

    // Captured task entrypoints passed into TaskRunner::start()
    common::StepFn httpFn = nullptr;
    void* httpUser = nullptr;
    common::StepFn playerFn = nullptr;
    void* playerUser = nullptr;

    std::unique_ptr<services::PlayerService> playerService;
};
