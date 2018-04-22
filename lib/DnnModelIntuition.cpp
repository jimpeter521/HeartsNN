// lib/DnnModelIntuition.cpp

#include "lib/DnnModelIntuition.h"
#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/random.h"
#include "lib/timer.h"

using namespace std;
using namespace tensorflow;

DnnModelIntuition::~DnnModelIntuition() { delete mPredictor; }

DnnModelIntuition::DnnModelIntuition(const std::string& modelPath, bool pooled)
    : mPredictor(0)
{
    using namespace tensorflow;
    SessionOptions session_options;
    RunOptions run_options;
    auto status = LoadSavedModel(session_options, run_options, modelPath, {kSavedModelTagServe}, &mModel);
    if (!status.ok())
    {
        std::cerr << "Failed: " << status;
        exit(1);
    }

    if (pooled)
        mPredictor = new PooledPredictor(mModel);
    else
        mPredictor = new SynchronousPredictor(mModel);
}

Card DnnModelIntuition::choosePlay(const KnowableState& state, const RandomGenerator& rng) const
{
    FloatMatrix matrix = state.AsFloatMatrix();
    Tensor mainData(DT_FLOAT, TensorShape({1, kCardsPerDeck, KnowableState::kNumFeaturesPerCard}));

    const float* srcData = matrix.data();
    float* dstData = mainData.flat<float>().data();

    memcpy(dstData, srcData, kCardsPerDeck * KnowableState::kNumFeaturesPerCard * sizeof(float));

    std::vector<tensorflow::Tensor> outputs;
    mPredictor->Predict(mainData, outputs);

    float playExpectedValue[13];
    return state.ParsePrediction(outputs, playExpectedValue);
}
