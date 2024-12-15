#include <Adafruit_NeoPixel.h>

//#include <Adafruit_NeoPixel.h>
#include <math.h>





// light strength 0 - 3
#define STRENGTH 3
#define EFFECTIVE_WIDTH 54

// Animations
#define MAX_ANIMATION 10





// LED positions
#define OUTDOOR 1

#define LINE_NUMBER 15
#define PIXELS_PER_LINE 60
#define PADDING 0

// 0 - 8. 0 means normal; +1 means double the animation speed.
#define FAST_FORWARD 1

uint16_t from_coordinate_to_number(byte x, byte y) {

  if (OUTDOOR) {
    //compensate the LEDs blocked by the window frames
    if (x >= 17 ) x += 3;
    if (x >= 40 ) x += 3;
  }
    

  if (x >= PIXELS_PER_LINE) return 0;
  if (y >= LINE_NUMBER) return 0;
  
  y = LINE_NUMBER -1 - y; // start of control is from bottom
  uint16_t p;
  if (y % 2 != OUTDOOR) {
    x = PIXELS_PER_LINE - 1 -x;
  }
  p = y * (PIXELS_PER_LINE + PADDING) + x;
  
  return p;
};

class Animation {
  private:
    Adafruit_NeoPixel* strip = NULL;    
  protected:
    byte min_y = 0, min_x = 0;
    byte width = 0, height = 0;
    unsigned long next_time = 0;
    void setPixelColor(byte x, byte y, uint32_t color) {
      if (x < width && y < height) {
        strip->setPixelColor(from_coordinate_to_number(x + min_x, y + min_y), color);
      }
    }
    uint32_t getPixelColor(byte x, byte y) {
      if (x < width && y < height) {    
        return strip->getPixelColor(from_coordinate_to_number(x + min_x, y + min_y));
      } else {
        return 0;        
      }
    }
    unsigned long refresh(unsigned long interval) {
      interval = interval >> FAST_FORWARD;
      strip->show();
      next_time = millis() + interval;       
    }
    
    
  public:  
    Animation(byte x1, byte y1, byte x2, byte y2) {
      min_x = x1;
      min_y = y1;
      width = x2 - min_x;
      height = y2 - min_y;
      next_time = millis();
    };
    unsigned long getShowTime() { return next_time; };
    static bool later(unsigned long a, unsigned long b) { 
      //a is later than b
      return a > b;
      return ((a - b) >> 31) == 0; // wrap-around safe
    }; 
    virtual void render() {};
    void setStrip(Adafruit_NeoPixel* newStrip) { strip = newStrip;};
    bool isActive() {
      return strip != NULL;
    };
    void deActivate() {
      strip = NULL;
    }
};



class AnimationPlayer {

  
// Controller configurations
#define PIN 6

  private:
    Animation* list[MAX_ANIMATION];
    byte num = 0;
    unsigned long duration = 0;
    unsigned long last_start = 0;
    bool showing = false;
    
    // Parameter 1 = number of pixels in strip
    // Parameter 2 = pin number (most are valid)
    // Parameter 3 = pixel type flags, add together as needed:
    //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    Adafruit_NeoPixel *strip = NULL; 

    
  public:
    AnimationPlayer() {      
      // Initialize all pixels to 'off'
      strip = new Adafruit_NeoPixel((PIXELS_PER_LINE * LINE_NUMBER), PIN, NEO_GRB + NEO_KHZ800);
      strip->begin();
      duration = 0;
      blackout();
    };
    void setPlayDuration(unsigned long d = 0) {
      duration = d;
    };
    
    Adafruit_NeoPixel* getStrip() { return strip;};
    
    bool isOn() {
      return showing;      
    };
    
    bool registerAnimation(Animation* a) {
      // true: successful
      // false: failed
      if (num >= MAX_ANIMATION) {
        return false;
      }
      list[num] = a;
      num ++;
      a->setStrip(strip);      
      return true;
    };


    unsigned long play() {
      // return the sleep time
      
      unsigned long now = millis();
      unsigned long next = now + ((unsigned long)1 << 30); // set the original value of next to be the furthest away from now.
      
      if (!showing) return 0;
      
      if (duration) {
        //currently showing. and there is a fixed duration
        if (Animation::later(now, last_start + duration)) {
          turnOff();
          return 0;
        }
      }

      int i = 0;
      while (i < num) {
        
        if (!list[i]->isActive()) {
          //clean up inactive animation
          delete list[i];
          Serial.println("Deleted");
          list[i] = list[num - 1];
          num --;
          continue;
        }
        
        if (Animation::later(now, list[i] -> getShowTime())) {          
          list[i] -> render();    
        }
        unsigned long local_next = list[i] -> getShowTime();
        if (Animation::later(next, local_next)) {
          next = local_next;
        }
        i ++;
      }
      now = millis();
      if (Animation::later(next, now)) return next - now; else return 0;
    };

   
    void turnOff() {
      if (!showing) return;
      blackout();
      showing = false;
    };
    
    void turnOn() {
      if (showing) return;
      unitTest(60);
      showing = true;
      last_start = millis();
    };
    
    void blackout() {
      for(uint16_t i=0; i<strip->numPixels(); i++) {
        strip->setPixelColor(i, Adafruit_NeoPixel::Color(0,   0,   0));
      }
      strip->show();  
    };
    
    void unitTest(int delta) {
      int led_number = PIXELS_PER_LINE * LINE_NUMBER;
      for (int i=0; i< led_number; i+= delta) {
        for (int j = 0; j < delta; j++) {
          if ( (i + j) < led_number) {
            strip->setPixelColor(i + j, Adafruit_NeoPixel::Color(0,   255,   0));
          } 
        }
        
        strip->show();
        for (int j = 0; j < delta; j++) {
          if ((i + j) < led_number ) {
            strip->setPixelColor(i + j, Adafruit_NeoPixel::Color(0,   0,   255));
          } 
        }
        strip->show();

        for (int j = 0; j < delta; j++) {
          if ( (i + j) < led_number ) {
            strip->setPixelColor(i + j, Adafruit_NeoPixel::Color(255,   0,   0));
          } 
        }
        strip->show();

        for (int j = 0; j < delta; j++) {
          if ( (i + j) < led_number ) {
            strip->setPixelColor(i + j, Adafruit_NeoPixel::Color(0,   0,   0));
          } 
        }
        strip->show();
      } 
    };
};





#define SNOW_NUM 6
class SnowAnimation: public Animation {
  private:
    byte snow_x[SNOW_NUM], snow_y[SNOW_NUM];  
    uint32_t color[SNOW_NUM];
    byte current_snow_num = 0;
    
  public:
  
  SnowAnimation(byte x1, byte y1, byte x2, byte y2) : Animation(x1, y1, x2, y2) {};
  
  virtual void render() {  
    int s = 0;
  
    while (s < current_snow_num) {
      setPixelColor(snow_x[s], snow_y[s], color[s]);
      snow_y[s] ++;
      if (snow_y[s] == height) {
        current_snow_num --; 
        snow_y[s] = snow_y[current_snow_num];
        snow_x[s] = snow_x[current_snow_num];
        color[s] = color[current_snow_num];
      } else {
        int dir = rand() % 3;
        if (dir == 1) {
          if (snow_x[s] < width -1 ) snow_x[s]++; 
        } else if (dir == 2) {
          if (snow_x[s]) snow_x[s]--;
        }
        color[s] = getPixelColor(snow_x[s], snow_y[s]);
        setPixelColor(snow_x[s], snow_y[s], Adafruit_NeoPixel::Color(255,   255,   255));
        s++;
      }
    }

    int new_snow = rand() % (SNOW_NUM - current_snow_num);    
    for (int i = 0; i< new_snow; i++) {
      snow_y[current_snow_num] = 0;
      snow_x[current_snow_num] = rand() % width;
      
      color[s] = getPixelColor(snow_x[current_snow_num], snow_y[current_snow_num]);
      setPixelColor(snow_x[current_snow_num], snow_y[current_snow_num], Adafruit_NeoPixel::Color(255,   255,   255));
      
      current_snow_num ++;
    }
    
    refresh(1500);
  }
};



class BaloonAnimation: public  Animation {
  private:

  byte current_y = 0, current_x = 0;
  bool background_recorded = false;

  uint32_t color;
  uint32_t pixels[5][3] = {
    {0, 1, 0 },
    {1, 1, 1 },
    {0, 1, 0 },
    {0, 1, 0 },
    {0, 1, 0 },
  };

  uint32_t background_color[5][3] = {
    {0, 1, 0 },
    {1, 1, 1 },
    {1, 1, 1 },
    {0, 1, 0 },
    {0, 1, 0 },
  };

  
  public:
  
  BaloonAnimation(byte x_pos) : Animation(x_pos-1, 0, x_pos+2, 5) {
    current_x = x_pos;

    switch (rand()%4) {
      case 0: 
        color = Adafruit_NeoPixel::Color(50 * STRENGTH, 17 * STRENGTH, 5 * STRENGTH); break;
      case 1: 
        color = Adafruit_NeoPixel::Color(0, 0, 17 * STRENGTH); break;
      case 2: 
        color = Adafruit_NeoPixel::Color(17 * STRENGTH, 17 * STRENGTH, 0); break;        
      default: 
        color = Adafruit_NeoPixel::Color(0, 17 * STRENGTH, 0);
    };
  };

  void ops(byte mode) {
    // mode: 0: record background; 1: repaint background; 2: paint
    for (int i = 0; i < 3; i++) 
      for (int j = 0; j < 5; j++) {
        if (mode == 0) {
          background_color[j][i] = getPixelColor(i, j);
        } else if (mode == 1) {
          setPixelColor(i, j, background_color[j][i]);
        } else if (mode == 2) {
          setPixelColor(i, j, pixels[j][i] * color);
        }
      }
  }
  virtual void render() {

    
    Serial.print(current_x);
    Serial.print(" ");
    Serial.println(current_y);

    if (!isActive()) return;

    
    // restore
    if (background_recorded) ops(1);
    


    // move
    current_y ++;
    if ((current_y + 5) >= LINE_NUMBER) { 
      deActivate();
      return;
    }
    byte dir = rand()%3;
    if (dir == 0 && current_x > 1) 
      current_x --;
    else if (dir == 2 && (current_x + 2) < EFFECTIVE_WIDTH) 
      current_x ++;

      

    // new position set up
    min_x = current_x - 1;
    min_y = current_y;

    // record
    ops(0);
    background_recorded = true;


    
    // paint
    // set the tail string 
    byte tail = rand() % 3;
    pixels[4][0] = 0; 
    pixels[4][1] = 0; 
    pixels[4][2] = 0; 
    pixels[4][tail] = 1;
    ops(2);
    refresh(7500);
  }  
  
};
class PlaneAnimation: public  Animation {
  private:
  AnimationPlayer* my_player = NULL;
  byte content_height = 5, content_width = 6, current_x = 0;
  byte parity = 0;
  int dir = 1;
  
  byte pixels[5][6] = {
    {0, 2, 1, 0, 0, 0 },
    {0, 0, 0, 1, 0, 0 },
    {1, 3, 1, 1, 1, 3 },
    {0, 0, 0, 1, 0, 0 },
    {0, 4, 1, 0, 0, 0 },
  };
  
  public:
  
  PlaneAnimation(byte x1, byte y1, byte x2, byte y2, AnimationPlayer* p) : Animation(x1, y1, x2, y2) { my_player = p;};
  
  virtual void render() {      
    for (int y = 0; y < content_height; y++) 
      for (int x = 0; x < content_width; x++) 
         setPixelColor(x + current_x, y, 0);
         
    if (parity % 4 == 0) {
      if (current_x == 0) {
        dir = 1; 
      } else if (current_x + content_width >=  width) {
        dir = -1;
      };      
      current_x += dir;
    }


    parity = (parity + 1) % 12;
    
    for (int y = 0; y < content_height; y++) 
      for (int x = 0; x < content_width; x++) {
        uint32_t color = 0;
        switch (pixels[dir > 0 ? y : content_height - 1 - y][dir > 0 ? x : content_width - 1 - x]) {
          case 0: color = 0; break;
          case 1: color = Adafruit_NeoPixel::Color(20, 20, 20); break; // fuselage
          case 4: color = Adafruit_NeoPixel::Color(255, 0, 0); break; // right wing light
          case 2: color = Adafruit_NeoPixel::Color(0, 255, 0); break; // left wing light
          case 3: color = (parity == 0 || parity == 3) ? Adafruit_NeoPixel::Color(255, 255, 255) : Adafruit_NeoPixel::Color(20, 20, 20);; break; // head light
          default: break;   
        }
        setPixelColor(x + current_x, y, color);
     }
     refresh(100);

/* 
     if ((current_x > 1) && (current_x + 1) < content_width) {
       if (rand() % 3 == 0) {
         my_player->registerAnimation(new BaloonAnimation(current_x));
       }
     }
*/
      //if (current_x == 25) my_player->registerAnimation(new BaloonAnimation(current_x - content_width * dir));
  }  
  
};





class CarAnimation: public  Animation {
  private:
  byte content_height = 3, content_width = 6, current_x = 2;
  int dir = 1;

  byte pixels[3][6] = {
    {0, 1, 1, 1, 0, 0},
    {3, 1, 1, 1, 1, 2 },
    {0, 1, 0, 0, 1, 0 },
  };

  byte light_height = 4, light_width = 2;
  byte current_light = 0;
  
  byte light_pixels_left[4][2] = {
    {1, 1}, 
    {1, 1}, 
    {0, 0}, 
    {0, 0}
  };

  byte light_pixels_right[4][2] = {
    {0, 0}, 
    {0, 0}, 
    {3, 3}, 
    {3, 3}
  };
  void changeLight(int new_light) {
    current_light = new_light;
    for (int i = 0; i < 4; i++) {
      byte color = 0;
      if (i == current_light || i == (current_light + 1)) {
        color = (current_light + 1);
      }
      for (int j = 0; j < 2; j++) {
        light_pixels_left[i][j] = color;
        light_pixels_right[3-i][j] = color ? 4 - color : 0;
      }
    }    
  }  
  public:
  
  CarAnimation(byte x1, byte y1, byte x2, byte y2) : Animation(x1, y1, x2, y2) {};
  
  virtual void render() {

    

    // Car
    for (int y = 0; y < content_height; y++) 
      for (int x = 0; x < content_width; x++) 
         setPixelColor(x + current_x, y, 0);
    if (current_x <= 2) {
        dir = 1; 
        changeLight(0);
    } else if (current_x + content_width >=  (width - 2)) {
      dir = -1;
      changeLight(2);
    } else {
      int predict_x = current_x;
      predict_x += dir * 10;
      if ( predict_x < 2 || (predict_x + content_width) > (width -2) ) {
        changeLight(1);
      }
    }
    current_x += dir;


    //Lights
    for (int y = 0; y < light_height; y++) 
      for (int x = 0; x < light_width; x++) {
        setPixelColor(x, y, 0);
        setPixelColor(width -2 + x, y, 0);
      }
         
    for (int y = 0; y < light_height; y++) 
      for (int x = 0; x < light_width; x++) {
        uint32_t color = 0;
        switch (light_pixels_left[y][x]) {
          case 0: color = 0; break;
          case 1: color = Adafruit_NeoPixel::Color(255, 0, 0); break; // Red Light
          case 2: color = Adafruit_NeoPixel::Color(255, 255, 0); break; // Yellow Light
          case 3: color = Adafruit_NeoPixel::Color(0, 255, 0);  break; // Green Light
          default: break;   
        }
        setPixelColor(x, y, color);

        switch (light_pixels_right[y][x]) {
          case 0: color = 0; break;
          case 1: color = Adafruit_NeoPixel::Color(255, 0, 0); break; // Red Light
          case 2: color = Adafruit_NeoPixel::Color(255, 255, 0); break; // Yellow Light
          case 3: color = Adafruit_NeoPixel::Color(0, 255, 0);  break; // Green Light
          default: break;   
        }
        setPixelColor(width - 2 + x, y, color);        
     }


    
    for (int y = 0; y < content_height; y++) 
      for (int x = 0; x < content_width; x++) {
        uint32_t color = 0;
        switch (pixels[y][dir > 0 ? x : content_width - 1 - x]) {
          case 0: color = 0; break;
          case 1: color = Adafruit_NeoPixel::Color(20, 20, 20); break; // body
          case 2: color = Adafruit_NeoPixel::Color(255, 255, 255); break; // headlight
          case 3: color = Adafruit_NeoPixel::Color(current_light == 2 ? 255 : 100, 0, 0);  break; // tail light
          default: break;   
        }
        setPixelColor(x + current_x, y+1, color);
     }
     if (current_light == 1) 
        refresh(1600);
     else 
        refresh(800);
  }  
  
};



class WordsAnimation: public Animation {
  
  
  private:
  

  const byte alphabet_width[26] = {
    3, 3, 3, 3, 3, 3, 5, 
    3, 3, 3, 3, 3, 5, 4, 
    3, 3, 3, 
    3, 3, 3,
    3, 3, 5,
    4, 3, 3,
   };
   const byte alphabet[26][5][5] = {
    { // A
      {0,1,0,0,0,},
      {1,0,1,0,0,},
      {1,1,1,0,0,},
      {1,0,1,0,0,},
      {1,0,1,0,0,},
    },
    { // B
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,1,0,0,0, },
    },
      
    { // C
      {0,1,1,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
      {0,1,1,0,0, },
    },
    { // D
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,1,0,0,0, },
    },
    { // E
      {1,1,1,0,0, },
      {1,0,0,0,0, },
      {1,1,1,0,0, },
      {1,0,0,0,0, },
      {1,1,1,0,0, },
    },    
    { // F
      {1,1,1,0,0, },
      {1,0,0,0,0, },
      {1,1,1,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
    },
    { // G
      {0,1,1,1,0, },
      {1,0,0,0,0, },
      {1,0,1,1,1, },
      {1,0,0,1,0, },
      {0,1,1,1,0, },
    },
    { // H
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,1,1,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
    },    
    { // I
      {1,1,1,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {1,1,1,0,0, },
    },
    { // J
      {1,1,1,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {1,0,0,0,0, },
    },        
    { // K
      {1,0,1,0,0, },
      {1,1,0,0,0, },
      {1,0,0,0,0, },
      {1,1,0,0,0, },
      {1,0,1,0,0, },
    },
    { // L
      {1,0,0,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
      {1,1,1,0,0, },
    },
    { // M
      {1,0,0,0,1,},
      {1,1,0,1,1,},
      {1,0,1,0,1,},
      {1,0,1,0,1,},
      {1,0,0,0,1,},
    },
    { // N
      {1,0,0,1,0, },
      {1,1,0,1,0, },
      {1,0,0,1,0, },
      {1,0,1,1,0, },
      {1,0,0,1,0, },
    },
    { // O
      {0,1,0,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {0,1,0,0,0, },
    },
    { // P
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,1,0,0,0, },
      {1,0,0,0,0, },
      {1,0,0,0,0, },
    },
    { // Q
      {0,1,0,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {0,1,0,0,0, },
      {0,0,1,0,0, },
    },
    { // R
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,1,0,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
    },    
    { // S
      {0,1,1,0,0, },
      {1,0,0,0,0, },
      {0,1,0,0,0, },
      {0,0,1,0,0, },
      {1,1,0,0,0, },
    },   
    { // T
      {1,1,1,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
      {0,1,0,0,0, },
    },    
    { // U
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,0,1,0,0, },
      {1,1,1,0,0, },
    },
    { // V
      {1,0,1,0,0 },
      {1,0,1,0,0 },
      {1,0,1,0,0 },
      {0,1,0,0,0 },
      {0,1,0,0,0},
    },
    { // W
      {1,0,0,0,1 },
      {1,0,1,0,1 },
      {1,0,1,0,1 },
      {1,1,0,1,1 },
      {1,0,0,0,1 },
    },
    { // X
      {1,0,0,1,0 },
      {1,0,0,1,0 },
      {0,1,1,0,0 },
      {1,0,0,1,0 },
      {1,0,0,1,0 },
    },
    { // Y
      {1,0,1,0,0 },
      {1,0,1,0,0 },
      {0,1,0,0,0 },
      {0,1,0,0,0 },
      {0,1,0,0,0 },
    },
    { // Z
      {1,1,1,0,0 },
      {0,0,1,0,0 },
      {0,1,0,0,0 },
      {1,0,0,0,0 },
      {1,1,1,0,0 },
    },
  };
  const byte number_width[10] = {3, 1, 3, 3, 3, 3, 3, 3, 3, 3};  
  const byte number[10][5][3] = {
    { // 0
      {1,1,1},
      {1,0,1},
      {1,0,1},
      {1,0,1},
      {1,1,1},
    },
    { // 1
      {1,0,0},
      {1,0,0},
      {1,0,0},
      {1,0,0},
      {1,0,0},
    },
    { // 2
      {1,1,1},
      {0,0,1},
      {1,1,1},
      {1,0,0},
      {1,1,1},
    },
    { // 3
      {1,1,1},
      {0,0,1},
      {1,1,1},
      {0,0,1},
      {1,1,1},
    },    
    
    { // 4
      {1,0,1},
      {1,0,1},
      {1,1,1},
      {0,0,1},
      {0,0,1},
    },        
    { // 5
      {1,1,1},
      {1,0,0},
      {1,1,1},
      {0,0,1},
      {1,1,1},
    },        
    { // 6
      {1,1,1},
      {1,0,0},
      {1,1,1},
      {1,0,1},
      {1,1,1},
    },  
    { // 7
      {1,1,1},
      {0,0,1},
      {0,0,1},
      {0,0,1},
      {0,0,1},
    },        
    { // 8
      {1,1,1},
      {1,0,1},
      {1,1,1},
      {1,0,1},
      {1,1,1},
    },  
    { // 9
      {1,1,1},
      {1,0,1},
      {1,1,1},
      {0,0,1},
      {0,0,1},
    },      
  };
  
  const char* content = NULL;
  const char* content_list = NULL;

  byte pattern = 0;
  byte last_width = PIXELS_PER_LINE;


  
  
  const byte getPixelsForCharactor(char a, byte x, byte y) {
    if (a == ' ') {
      return 0;
    } else if (a >= 'a' && a <= 'z') {
      return (alphabet[a - 'a'][y][x]);
    } else if (a >= '0' && a <= '9') {
      return (number[a - '0'][y][x]);
    } else if (a >= 'A' && a <= 'Z') {
      return (alphabet[a - 'A'][y][x]);
    } else {
      return 0;
    };   
  }
  const byte getLengthForCharacter(char a) {
    if (a == ' ') {
      return 1;
    } else if (a >= 'a' && a <= 'z') {
      return (alphabet_width[a - 'a']);
    } else if (a >= '0' && a <= '9') {
      return (number_width[a - '0']);
    } else if (a >= 'A' && a <= 'Z') {
      return (alphabet_width[a - 'A']);
    } else {
      return 0;
    };
  }
  
  

  public:
  WordsAnimation(byte x1, byte y1, byte x2, byte y2, const char* s) : Animation(x1, y1, x2, y2) {
    //const char* s: set of strings to be displayed, separated by \n
    content = NULL;
    content_list = s;
    getNextContent();
  };

  void getNextContent() {
    
    if (*content == 0) {
      content = content_list; 
    } else {
      while ((*content) && (*content) != '\n') {
        content++;
      }
      if (*content) content ++;
      if (*content == 0) {
        content = content_list;
      }
    }
  }
  
  
  virtual void render() {  
    
    uint32_t color = 0;
    switch (pattern) {

      case 0: 
        getNextContent();
        color = Adafruit_NeoPixel::Color(50 * STRENGTH, 17 * STRENGTH, 5 * STRENGTH); break;
      /*case 1: 
        color = Adafruit_NeoPixel::Color(0,   50 * STRENGTH,  0); break;
      case 2:
        color = Adafruit_NeoPixel::Color(75 * STRENGTH,  0, 5 * STRENGTH); break;
      case 3: 
        color = Adafruit_NeoPixel::Color(17 * STRENGTH, 17 * STRENGTH, 17 * STRENGTH); break;
        */
      case 1: 
        color = Adafruit_NeoPixel::Color(0, 0, 17 * STRENGTH); break;
      case 2: 
        color = Adafruit_NeoPixel::Color(17 * STRENGTH, 17 * STRENGTH, 0); break;        
      default: 
        pattern = 127;
        break;
    }
    if (pattern == 127) pattern = 0; else pattern = pattern + 1;

    const char* c = content;
    byte current_pos = 0;
    while ((*c) && (*c) != '\n') {
      if (current_pos) {
        for (int y = 0; y < 5; y++) {
          setPixelColor(current_pos, y, 0);
        }
        current_pos++;
      }
      for (int y = 0; y < 5; y++) 
        for (int x = 0; x < getLengthForCharacter(*c); x++)
           setPixelColor(current_pos + x, y, getPixelsForCharactor(*c, x, y) ? color : 0);
      current_pos += getLengthForCharacter(*c);
      c ++;
    }

    
    if (current_pos < last_width) {
      // the last display is wider than this one, let's clean up the rest of the width
      for (int y = 0; y < 5; y++)
        for (int x = current_pos; x < last_width; x ++) {
          setPixelColor(x, y, 0);
        }
    }
    // reset the width of last display
    last_width = current_pos;
         
    refresh(10000);
  }
};


class DayNightController {

  
#define LIGHT_PIN A7
#define LIGHT_THRESHOLD 512
#define CHECK_FREQUENCY 1000
#define CHANGE_DELAY 10

  public:
    bool isOn() {
      checkChange();
      return !is_daylight;
    };
    DayNightController () {
      is_daylight = false;
      last_check_time = 0;
      count_down = CHANGE_DELAY;
      checkChange();
    };

    
  private:
    bool is_daylight;
    int count_down;
    unsigned long last_check_time;
    void checkChange() {
      unsigned long now = millis();
      if (!Animation::later(now, last_check_time + CHECK_FREQUENCY)) return;
      last_check_time = now;

      unsigned int light_level = analogRead(LIGHT_PIN);
      bool this_sample = light_level > LIGHT_THRESHOLD; // the higher the reading, the more light (max 5V / 1023)
 
      if (this_sample != is_daylight) {
        if (count_down == 0) {
          is_daylight = this_sample;
          count_down = CHANGE_DELAY;
        } else {
          count_down --;
        }
      } else {
        count_down = CHANGE_DELAY;
      }

#ifdef DEBUG_ON
      Serial.print("read: ");
      Serial.print(light_level);
      Serial.print(" light on: ");
      Serial.print(this_sample, BIN);
      Serial.print(" count down: ");
      Serial.print(count_down);      
      Serial.print(" next check: ");
      Serial.println(last_check_time + CHECK_FREQUENCY );
#endif
    };
};
    
    
    
AnimationPlayer* player;

DayNightController* day_night_controller;
  
void setup() {  
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif

  Serial.begin(9600);
  while(!Serial) ;

  day_night_controller = new DayNightController();
  player = new AnimationPlayer();
  player->registerAnimation(new WordsAnimation(0, 6, EFFECTIVE_WIDTH, 11, "HAPPY HOLIDAYS"));
  player->registerAnimation(new SnowAnimation(0, 0, EFFECTIVE_WIDTH, 15));  
  player->registerAnimation(new PlaneAnimation(0, 0, EFFECTIVE_WIDTH, 5, player));
  player->registerAnimation(new CarAnimation(0, 11, EFFECTIVE_WIDTH, 15));
  //player->registerAnimation(new BaloonAnimation(25));
}

void loop() {  
  unsigned long d; 
  if (!day_night_controller->isOn()) {
    if (player->isOn()) {
      player->turnOff();
    };
  } else {
    if (!player->isOn()) {
      player->turnOn();
    };
  };

  // next check
  if (player->isOn()) {
    d = player->play();
  } else {
    d = CHECK_FREQUENCY;
  };
  if (d) delay(d);  
  
}



