#pragma once

#include "lib/CardArray.h"
#include "lib/Predictor.h"
#include "lib/Strategy.h"

#include <tensorflow/cc/framework/ops.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/protobuf/meta_graph.pb.h>
#include <tensorflow/core/public/session.h>

class DnnModelIntuition : public Strategy
{
public:
    virtual ~DnnModelIntuition();

    DnnModelIntuition(const std::string& modelPath, bool pooled = false);

    virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

    virtual Card predictOutcomes(
        const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const;

private:
    tensorflow::SavedModelBundle mModel;
    Predictor* mPredictor;
};
