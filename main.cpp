#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS    10
#define TFT_DC    9
#define TFT_RST   8

#define PIN1_CLK  2
#define PIN1_DT   5
#define PIN1_SW   4
#define PIN2_CLK  3
#define PIN2_DT   7
#define PIN2_SW   12

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const int SCREEN_WIDTH  = 320;
const int SCREEN_HEIGHT = 240;
const int BALL_RADIUS   = 5;

// Arena
const int ARENA_TOP    = 32;
const int ARENA_BOTTOM = 236;
const int GOAL_TOP     = 60;
const int GOAL_BOTTOM  = 208;

// Bal status
float ballX = 160.0;
float ballY = 120.0;
float oldBallX = 160.0;
float oldBallY = 120.0;
float ballSpeedX = 2.0;
float ballSpeedY = 1.5;

// Timing (60 FPS)
unsigned long lastFrameTime = 0;
const int FRAME_INTERVAL = 16;

// Paddle afmetingen (verticaal)
const int PADDLE_WIDTH  = 8;
const int PADDLE_HEIGHT = 45;
const float MAX_BOUNCE_SPEED_Y = 4.0;

// Speler 1 (Links)
const int PADDLE1_X = 10;
int paddle1Y = 97;
int oldPaddle1Y = 97;

// Speler 2 (Rechts)
const int PADDLE2_X = 302;
int paddle2Y = 97;
int oldPaddle2Y = 97;

// Scores
int scoreP1 = 0;
int scoreP2 = 0;

// Encoder variabelen
int lastClk1State;
int lastClk2State;
volatile int counter1 = 0;
volatile int counter2 = 0;
int lastCounter1 = 0;
int lastCounter2 = 0;

// Interrupt Service Routine: Speler 1
void checkEncoder1() {
  int clk1State = digitalRead(PIN1_CLK);
  int dt1State = digitalRead(PIN1_DT);

  if (clk1State != lastClk1State && clk1State == LOW) {
    if (dt1State != clk1State) {
      counter1++; 
    } 
    else {
      counter1--; 
    }
  }
  lastClk1State = clk1State;
}

// Interrupt Service Routine: Speler 2
void checkEncoder2() {
  int clk2State = digitalRead(PIN2_CLK);
  int dt2State = digitalRead(PIN2_DT);

  if (clk2State != lastClk2State && clk2State == LOW) {
    if (dt2State != clk2State) {
      counter2++; 
    } 
    else {
      counter2--; 
    }
  }
  lastClk2State = clk2State;
}

void drawArena() {
  // Boven- en onderrand van het speelveld (dikte 2 px)
  tft.fillRect(0, ARENA_TOP, SCREEN_WIDTH, 2, ILI9341_WHITE);
  tft.fillRect(0, ARENA_BOTTOM, SCREEN_WIDTH, 2, ILI9341_WHITE);

  // Linker hoekwanden (boven en onder het doel)
  tft.fillRect(0, ARENA_TOP, 4, GOAL_TOP - ARENA_TOP, ILI9341_CYAN);
  tft.fillRect(0, GOAL_BOTTOM, 4, ARENA_BOTTOM - GOAL_BOTTOM, ILI9341_CYAN);

  // Rechter hoekwanden (boven en onder het doel)
  tft.fillRect(SCREEN_WIDTH - 4, ARENA_TOP, 4, GOAL_TOP - ARENA_TOP, ILI9341_MAGENTA);
  tft.fillRect(SCREEN_WIDTH - 4, GOAL_BOTTOM, 4, ARENA_BOTTOM - GOAL_BOTTOM, ILI9341_MAGENTA);

  // Gestippeld net in het midden
  for (int y = ARENA_TOP + 6; y < ARENA_BOTTOM; y += 12) {
    tft.drawFastVLine(160, y, 6, ILI9341_DARKGREY);
  }
}

void updateScoreboard() {
  tft.setTextSize(2);
  // Speler 1 (Links)
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setCursor(60, 8);
  tft.print("P1: ");
  tft.print(scoreP1);

  // Speler 2 (Rechts)
  tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
  tft.setCursor(200, 8);
  tft.print("P2: ");
  tft.print(scoreP2);
}

void resetBall(int serveDirection) {
  ballX = 160.0;
  ballY = 120.0;
  ballSpeedX = serveDirection * 3.0; // Schiet richting de winnaar/verliezer
  ballSpeedY = random(-20, 20) / 10.0; // Lichte willekeurige verticale hoek
  tone(6, 150, 150);
}

void checkPaddleCollisions() {
  // 1. Botsing met linker paddle (Speler 1)
  if (ballX - BALL_RADIUS <= PADDLE1_X + PADDLE_WIDTH && ballX + BALL_RADIUS >= PADDLE1_X && ballSpeedX < 0) {
        
    if (ballY >= paddle1Y && ballY <= paddle1Y + PADDLE_HEIGHT) {

      ballX = PADDLE1_X + PADDLE_WIDTH + BALL_RADIUS;

      float paddleCenter = paddle1Y + (PADDLE_HEIGHT / 2.0);
      float hitOffset = (ballY - paddleCenter) / (PADDLE_HEIGHT / 2.0);
      hitOffset = constrain(hitOffset, -1.0, 1.0);
            
      ballSpeedY = hitOffset * MAX_BOUNCE_SPEED_Y;
      ballSpeedX = abs(ballSpeedX) * 1.05; // Kaats naar rechts
      tone(6, 800, 20);

      tft.fillRect(PADDLE1_X, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
    }
  }

  // 2. Botsing met rechter paddle (Speler 2)
  if (ballX + BALL_RADIUS >= PADDLE2_X && ballX - BALL_RADIUS <= PADDLE2_X + PADDLE_WIDTH && ballSpeedX > 0) {
        
    if (ballY >= paddle2Y && ballY <= paddle2Y + PADDLE_HEIGHT) {
      ballX = PADDLE2_X - PADDLE_WIDTH - BALL_RADIUS;

      float paddleCenter = paddle2Y + (PADDLE_HEIGHT / 2.0);
      float hitOffset = (ballY - paddleCenter) / (PADDLE_HEIGHT / 2.0);
      hitOffset = constrain(hitOffset, -1.0, 1.0);
            
      ballSpeedY = hitOffset * MAX_BOUNCE_SPEED_Y;
      ballSpeedX = -abs(ballSpeedX) * 1.05; // Kaats naar links
      tone(6, 800, 20);

      // FIX 2: Zorg dat de rechter paddle direct hersteld wordt
      tft.fillRect(PADDLE2_X, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
    }
  }
}

void updateBall() {
  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // Plafond & Vloer van de Arena
  if (ballY - BALL_RADIUS <= ARENA_TOP + 2) {
    ballY = ARENA_TOP + 2 + BALL_RADIUS;
    ballSpeedY = -ballSpeedY;
    tone(6, 440, 15);
  } 
  else if (ballY + BALL_RADIUS >= ARENA_BOTTOM) {
    ballY = ARENA_BOTTOM - BALL_RADIUS;
    ballSpeedY = -ballSpeedY;
    tone(6, 440, 15);
  }

  // Zijwanden: als de bal buiten het doel de hoekwanden raakt
  bool inGoalY = (ballY >= GOAL_TOP && ballY <= GOAL_BOTTOM);

  // Linker wand botsing (alleen buiten het doel)
  if (!inGoalY && ballX - BALL_RADIUS <= 4 && ballSpeedX < 0) {
    ballX = 4 + BALL_RADIUS;
    ballSpeedX = -ballSpeedX;
    tone(6, 440, 15);
  }
  // Rechter wand botsing (alleen buiten het doel)
  if (!inGoalY && ballX + BALL_RADIUS >= SCREEN_WIDTH - 4 && ballSpeedX > 0) {
    ballX = SCREEN_WIDTH - 4 - BALL_RADIUS;
    ballSpeedX = -ballSpeedX;
    tone(6, 440, 15);
  }

  // Score detectie: Bal vliegt door het doel naar buiten
  if (ballX < 0) {
    scoreP2++;
    updateScoreboard();
    resetBall(1);
  } 
  else if (ballX > SCREEN_WIDTH) {
    scoreP1++;
    updateScoreboard();
    resetBall(-1);
  }
}

void renderBall() {
  tft.fillCircle((int)oldBallX, (int)oldBallY, BALL_RADIUS, ILI9341_BLACK);

  // FIX 1: Herstel het net als de oude bal in de buurt van de middellijn was
  if (oldBallX >= 160 - BALL_RADIUS - 2 && oldBallX <= 160 + BALL_RADIUS + 2) {
    int startY = max(ARENA_TOP + 6, (int)oldBallY - BALL_RADIUS - 4);
    int endY   = min(ARENA_BOTTOM - 6, (int)oldBallY + BALL_RADIUS + 4);

    // Loop over de streepjes die binnen het bereik van de bal vielen
    for (int y = ARENA_TOP + 6; y < ARENA_BOTTOM; y += 12) {
      if (y + 6 >= startY && y <= endY) {
        tft.drawFastVLine(160, y, 6, ILI9341_DARKGREY);
      }
    }
  }

  // FIX 2: Herstel boven- en ondermuren bij botsingen/overlap
  if (oldBallY - BALL_RADIUS <= ARENA_TOP + 2) {
    tft.fillRect((int)oldBallX - BALL_RADIUS, ARENA_TOP, (BALL_RADIUS * 2) + 1, 2, ILI9341_WHITE);
  }
  if (oldBallY + BALL_RADIUS >= ARENA_BOTTOM) {
    tft.fillRect((int)oldBallX - BALL_RADIUS, ARENA_BOTTOM, (BALL_RADIUS * 2) + 1, 2, ILI9341_WHITE);
  }

  // FIX 3: Herstel zijhoekmuren (boven/onder het doel)
  if (oldBallX - BALL_RADIUS <= 4) { // Linker hoekwand
    if (oldBallY <= GOAL_TOP) {
      tft.fillRect(0, ARENA_TOP, 4, GOAL_TOP - ARENA_TOP, ILI9341_CYAN);
    } 
    else if (oldBallY >= GOAL_BOTTOM) {
      tft.fillRect(0, GOAL_BOTTOM, 4, ARENA_BOTTOM - GOAL_BOTTOM, ILI9341_CYAN);
    }
  }
  if (oldBallX + BALL_RADIUS >= SCREEN_WIDTH - 4) { // Rechter hoekwand
    if (oldBallY <= GOAL_TOP) {
      tft.fillRect(SCREEN_WIDTH - 4, ARENA_TOP, 4, GOAL_TOP - ARENA_TOP, ILI9341_MAGENTA);
    } 
    else if (oldBallY >= GOAL_BOTTOM) {
      tft.fillRect(SCREEN_WIDTH - 4, GOAL_BOTTOM, 4, ARENA_BOTTOM - GOAL_BOTTOM, ILI9341_MAGENTA);
    }
  }

  // 2. Teken de nieuwe bal
  tft.fillCircle((int)ballX, (int)ballY, BALL_RADIUS, ILI9341_WHITE);

  // Onthoud positie
  oldBallX = ballX;
  oldBallY = ballY;
}

void renderPaddles() {
  // Speler 1 (Links) bijwerken
  if (paddle1Y != oldPaddle1Y) {
    tft.fillRect(PADDLE1_X, oldPaddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_BLACK);
    tft.fillRect(PADDLE1_X, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
    oldPaddle1Y = paddle1Y;
  }

  // Speler 2 (Rechts) bijwerken
  if (paddle2Y != oldPaddle2Y) {
    tft.fillRect(PADDLE2_X, oldPaddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_BLACK);
    tft.fillRect(PADDLE2_X, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
    oldPaddle2Y = paddle2Y;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN1_CLK, INPUT);
  pinMode(PIN1_DT, INPUT);
  pinMode(PIN1_SW, INPUT_PULLUP);
  pinMode(PIN2_CLK, INPUT);
  pinMode(PIN2_DT, INPUT);
  pinMode(PIN2_SW, INPUT_PULLUP);

  lastClk1State = digitalRead(PIN1_CLK);
  lastClk2State = digitalRead(PIN2_CLK);

  attachInterrupt(digitalPinToInterrupt(PIN1_CLK), checkEncoder1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN2_CLK), checkEncoder2, CHANGE);

  tft.begin();
  tft.setRotation(3); 
  tft.fillScreen(ILI9341_BLACK);

  drawArena();
  updateScoreboard();

  // Teken beginposities
  tft.fillRect(PADDLE1_X, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
  tft.fillRect(PADDLE2_X, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, ILI9341_WHITE);
}

void loop() {
  // Encoder 1 (Speler 1 - Links)
  if (counter1 != lastCounter1) {
    paddle1Y += (counter1 - lastCounter1) * 8;
    paddle1Y = constrain(paddle1Y, GOAL_TOP, GOAL_BOTTOM - PADDLE_HEIGHT);
    lastCounter1 = counter1;
  }

  // Encoder 2 (Speler 2 - Rechts)
  if (counter2 != lastCounter2) {
    paddle2Y += (counter2 - lastCounter2) * 8;
    paddle2Y = constrain(paddle2Y, GOAL_TOP, GOAL_BOTTOM - PADDLE_HEIGHT);
    lastCounter2 = counter2;
  }

  // Vaste frame-interval (60 FPS)
  unsigned long currentMillis = millis();
  if (currentMillis - lastFrameTime >= FRAME_INTERVAL) {
    lastFrameTime = currentMillis;

    updateBall();
    checkPaddleCollisions();
    renderBall();
    renderPaddles();
  }
}