#ifndef OPPONENT_H
#define OPPONENT_H

#include "SDL.h"
#include "snake.h"
#include <memory>

class OpponentPolicy {
 private:
    int grid_width;
    int grid_height;

 protected:
    static constexpr std::array<Snake::Direction, 4> actions{Snake::Direction::kDown, 
                                                          Snake::Direction::kLeft, 
                                                          Snake::Direction::kRight,
                                                          Snake::Direction::kUp};

    Snake::Direction GetOppositeDirection(Snake::Direction direction) const;

    float CalculateDistanceToFood(float head_x, float head_y, const SDL_Point& food) const;

 public:
    OpponentPolicy() = delete;
    OpponentPolicy(int width, int height) : grid_width(width), grid_height(height) {} 
    
    virtual Snake::Direction Update(const Snake& ego, const Snake& opponent, const SDL_Point& food) {
        return Snake::Direction::kDown;
    }

    virtual ~OpponentPolicy() = default;
};

class GreedyPolicy : public OpponentPolicy {
 public:
    using OpponentPolicy::OpponentPolicy;

    Snake::Direction Update(const Snake& ego, const Snake& opponent, const SDL_Point& food) override;
};

class MinimaxPolicy : public OpponentPolicy {
 public:
    using OpponentPolicy::OpponentPolicy;

    Snake::Direction Update(const Snake& ego, const Snake& opponent, const SDL_Point& food) override;
    
 private:
    float ExpandTree(const Snake& ego, const Snake& opponent, const SDL_Point& food, int depth, bool maximizing) const;

    float EvaluateReward(const Snake& ego, const Snake& opponent, const SDL_Point& food, const int depth) const;

    static constexpr int MAX_SEARCH_DEPTH = 8;
};

// TODO: More advanced control strategies like Hybrid-A* or Monte-Carlo-Tree-Search

class Opponent : public Snake {
 public:
    Opponent() = delete;
    Opponent(int grid_width, int grid_height) 
        : Snake(grid_width, grid_height), policy(std::make_unique<GreedyPolicy>(grid_width, grid_height)) {}

    void Update(const Snake& opponent, SDL_Point& food);

    void Respawn(int x, int y);

    void UpdatePolicy(std::unique_ptr<OpponentPolicy> new_policy) {
        policy = std::move(new_policy);
    }

 private:
    std::unique_ptr<OpponentPolicy> policy;
};

#endif // OPPONENT_H