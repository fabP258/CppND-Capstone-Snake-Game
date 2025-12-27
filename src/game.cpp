#include "game.h"
#include "SDL.h"

Game::Game(std::size_t grid_width, std::size_t grid_height)
    : snake(grid_width, grid_height),
      opponent(grid_width, grid_height),
      engine(dev()),
      random_w(0, static_cast<int>(grid_width - 1)),
      random_h(0, static_cast<int>(grid_height - 1)) {
  PlaceFood();
  opponent.alive = false;
}

void Game::Run(Controller const &controller, Renderer &renderer,
               std::size_t target_frame_duration) {
  Uint32 title_timestamp = SDL_GetTicks();
  Uint32 frame_start;
  Uint32 frame_end;
  Uint32 frame_duration;
  int frame_count = 0;
  bool running = true;

  while (running) {
    frame_start = SDL_GetTicks();

    // Input, Update, Render - the main game loop.
    controller.HandleInput(running, snake);
    Update();
    renderer.Render(snake, opponent, food);

    frame_end = SDL_GetTicks();

    // Keep track of how long each loop through the input/update/render cycle
    // takes.
    frame_count++;
    frame_duration = frame_end - frame_start;

    // After every second, update the window title.
    if (frame_end - title_timestamp >= 1000) {
      renderer.UpdateWindowTitle(score, frame_count);
      frame_count = 0;
      title_timestamp = frame_end;
    }

    // If the time for this frame is too small (i.e. frame_duration is
    // smaller than the target ms_per_frame), delay the loop to
    // achieve the correct frame rate.
    if (frame_duration < target_frame_duration) {
      SDL_Delay(target_frame_duration - frame_duration);
    }
  }
}

void Game::PlaceFood() {
  GetRandomPosition(food.x, food.y);
}

void Game::GetRandomPosition(int& x, int& y) {
  while (true) {
    x = random_w(engine);
    y = random_h(engine);
    if (!snake.SnakeCell(x, y)) return;
  }
}

void Game::RespawnOpponent() {
  int x, y;
  GetRandomPosition(x, y);
  opponent.Respawn(x, y);
}

void Game::CheckForFoodAndCollision(Snake& ego, Snake& op, bool is_ego_opponent) {
  int new_x = static_cast<int>(ego.head_x);
  int new_y = static_cast<int>(ego.head_y);

  if (food.x == new_x && food.y == new_y) {
    PlaceFood();
    ego.GrowBody();
    ego.speed = std::min(ego.speed + SPEED_INCREMENT, MAX_SPEED);
    if (!is_ego_opponent) {
      score++;
    }
  }

  bool collision = op.alive && op.SnakeCell(new_x, new_y);
  ego.alive = ego.alive && !collision;
}

void Game::UpdateSnake() {
  if (!snake.alive) return;
  snake.Update();
  CheckForFoodAndCollision(snake, opponent, false);
}

void Game::UpdateOpponent() {
  if (!opponent.alive) return;
  opponent.Update(snake, food);
  CheckForFoodAndCollision(opponent, snake, true);
  if (!opponent.alive) {
    RespawnOpponent();
  }
}

void Game::Update() {
  UpdateSnake();
  if (!opponent.alive && score >= OPPONENT_ACTIVATION_SCORE) {
    RespawnOpponent();
  }
  if (score == MINIMAX_ACTIVATION_SCORE) {
    opponent.UpdatePolicy(std::move(std::make_unique<MinimaxPolicy>(snake.GridWidth(), snake.GridHeight())));
  }
  UpdateOpponent();
}

int Game::GetScore() const { return score; }
int Game::GetSize() const { return snake.size; }