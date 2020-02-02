
include_directories(${PROJECT_SOURCE_DIR})
add_library(core_lib STATIC
    Annotator.cpp
    Card.cpp
    CardArray.cpp
    Deal.cpp
    Distribution.cpp
    DnnModelIntuition.cpp
    DnnMonteCarloAnnotator.cpp
    GameOutcome.cpp
    GameState.cpp
    HeartsState.cpp
    HumanPlayer.cpp
    KnowableState.cpp
    MonteCarlo.cpp
    NoVoidsAnalyzer.cpp
    OneOpponentGetsSuit.cpp
    PossibilityAnalyzer.cpp
    Predictor.cpp
    RandomStrategy.cpp
    Semaphore.cpp
    Strategy.cpp
    Tournament.cpp
    TwoOpponentsGetSuit.cpp
    VoidBits.cpp
    WriteDataAnnotator.cpp
    WriteTrainingDataSets.cpp
    combinatorics.cpp
    debug.cpp
    math.cpp
    random.cpp
    timer.cpp
)

target_link_libraries(core_lib TensorflowCC::Static dlib::dlib)
