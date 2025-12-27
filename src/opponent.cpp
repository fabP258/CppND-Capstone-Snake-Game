#include "opponent.h"
#include <cmath>
#include <optional>
#include <future>
#include <vector>

Snake::Direction OpponentPolicy::GetOppositeDirection(Snake::Direction direction) const {
    switch (direction) {
      case Snake::Direction::kDown:
        return Snake::Direction::kUp;
      case Snake::Direction::kUp:
        return Snake::Direction::kDown;
      case Snake::Direction::kLeft:
        return Snake::Direction::kRight;
      case Snake::Direction::kRight:
        return Snake::Direction::kLeft;
    }
}

Snake::Direction GreedyPolicy::Update(const Snake& ego, const Snake& opponent, const SDL_Point& food) {
    Snake::Direction greedyAction;
    std::optional<float> minDistance = std::nullopt;
    for (auto& currentDirection : actions) {
      if (ego.size > 1 && ego.direction == GetOppositeDirection(currentDirection)) continue;
      float head_x = ego.head_x;
      float head_y = ego.head_y;
      switch (currentDirection) {
        case Snake::Direction::kUp:
          head_y -= ego.speed;
          break;

        case Snake::Direction::kDown:
          head_y += ego.speed;
          break;

        case Snake::Direction::kLeft:
          head_x -= ego.speed;
          break;

        case Snake::Direction::kRight:
          head_x += ego.speed;
          break;
        }

        head_x = fmod(head_x + ego.GridWidth(), ego.GridWidth());
        head_y = fmod(head_y + ego.GridHeight(), ego.GridHeight());
            
        float distance = CalculateDistanceToFood(head_x, head_y, food);

        if (!minDistance || distance < minDistance.value() - 0.001f || std::abs(distance - minDistance.value()) < 0.001f && currentDirection == ego.direction) {
          minDistance = distance;
          greedyAction = currentDirection;
        }
    }

    return greedyAction;
}

float OpponentPolicy::CalculateDistanceToFood(float head_x, float head_y, const SDL_Point& food) const {
  if (static_cast<int>(head_x) == food.x && static_cast<int>(head_y) == food.y) {
    return 0.0f;
  }

  float dx = std::abs(head_x - food.x);
  float dy = std::abs(head_y - food.y);

  // handle wrap around
  if (dx > grid_width / 2) dx = grid_width - dx;
  if (dy > grid_height / 2) dy = grid_height - dy;

  return dx + dy;
}

Snake::Direction MinimaxPolicy::Update(const Snake& ego, const Snake& opponent, const SDL_Point& food) {

  struct MoveResult {
    Snake::Direction dir;
    std::future<float> reward;
    std::thread thr;
  };

  std::vector<MoveResult> move_results;

  for (Snake::Direction current_direction : actions) {
    if (ego.size > 1 && ego.direction == GetOppositeDirection(current_direction)) continue;

    Snake ego_copy = ego;
    ego_copy.direction = current_direction;
    ego_copy.Update();
    ego_copy.CheckForColission(opponent);

    std::promise<float> reward_promise;
    std::future<float> reward_future = reward_promise.get_future();

    move_results.push_back({
      current_direction,
      std::move(reward_future),
      std::thread([this, ego_copy, opponent, food](std::promise<float> p) {
        float result = this->ExpandTree(ego_copy, opponent,food, 1, false);
        p.set_value(result);
      }, std::move(reward_promise))
    });
  }

  Snake::Direction best_action;
  float max_reward = -std::numeric_limits<float>::infinity();
  
  for (auto& result : move_results) {
    float current_reward = result.reward.get();

    if (result.thr.joinable()) {
      result.thr.join();
    }

    if (current_reward > max_reward) {
      max_reward = current_reward;
      best_action = result.dir;
    }
  }

  return best_action;
}

float MinimaxPolicy::ExpandTree(const Snake& ego, const Snake& opponent, const SDL_Point& food, int depth, bool maximizing) const {
  bool food_claimed = ego.HeadCell(food.x, food.y) || opponent.HeadCell(food.x, food.y);
  if (depth == MAX_SEARCH_DEPTH || !ego.alive || !opponent.alive || food_claimed) {
    return EvaluateReward(ego, opponent, food, depth);
  }

  if (maximizing) {
    float max_reward = -std::numeric_limits<float>::infinity();
    for (auto move : actions) {
      if (ego.size > 1 && ego.direction == GetOppositeDirection(move)) continue;
      Snake ego_copy = ego;
      ego_copy.direction = move;
      ego_copy.Update();
      ego_copy.CheckForColission(opponent);
      float current_reward = ExpandTree(ego_copy, opponent, food, depth+1, false);
      max_reward = std::max(max_reward, current_reward);
    }
    return max_reward;
  }

  float min_reward = std::numeric_limits<float>::infinity();
  for (auto move : actions) {
    if (opponent.size > 1 && opponent.direction == GetOppositeDirection(move)) continue;
    Snake opponent_copy = opponent;
    opponent_copy.direction = move;
    opponent_copy.Update();
    opponent_copy.CheckForColission(ego);
    float current_reward = ExpandTree(ego, opponent_copy, food, depth+1, true);
    min_reward = std::min(min_reward, current_reward);
  }
  return min_reward;

}

float MinimaxPolicy::EvaluateReward(const Snake& ego, const Snake& opponent, const SDL_Point& food, const int depth) const {
  float reward = 0.0f;
  if (!ego.alive) {
    reward -= 100.0f + static_cast<float>(depth);
  }
  reward -= CalculateDistanceToFood(ego.head_x, ego.head_y, food);
  if (food.x == static_cast<int>(ego.head_x) && food.y == static_cast<int>(ego.head_y)) {
    reward += 100.0f - static_cast<float>(depth);
  }
  return reward;
}

void Opponent::Update(const Snake& opponent, SDL_Point& food) {
  if (policy) {
    direction = policy->Update(*this, opponent, food);
  }
  Snake::Update();
}

void Opponent::Respawn(int x, int y) {
  alive = true;
  head_x = x;
  head_y = y;
  speed = 0.1f;
  body.clear();
}