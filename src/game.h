#ifndef GAME_H
#define GAME_H

#include <random>
#include "SDL.h"
#include "controller.h"
#include "renderer.h"
#include "snake.h"
#include "opponent.h"

class Game {
 public:
  Game(std::size_t grid_width, std::size_t grid_height);
  void Run(Controller const &controller, Renderer &renderer,
           std::size_t target_frame_duration);
  int GetScore() const;
  int GetSize() const;

 private:
  Snake snake;
  Opponent opponent;
  SDL_Point food;

  std::random_device dev;
  std::mt19937 engine;
  std::uniform_int_distribution<int> random_w;
  std::uniform_int_distribution<int> random_h;

  static constexpr int OPPONENT_ACTIVATION_SCORE = 4;
  static constexpr float SPEED_INCREMENT = 0.01f;
  static constexpr float MAX_SPEED = 0.5f;
  static constexpr int MINIMAX_ACTIVATION_SCORE = 8;

  int score{0};

  void PlaceFood();
  void GetRandomPosition(int& x, int& y);
  void RespawnOpponent();
  void Update();
  void UpdateSnake();
  void UpdateOpponent();
  void CheckForFoodAndCollision(Snake& ego, Snake& op, bool is_ego_opponent);
};

#endif