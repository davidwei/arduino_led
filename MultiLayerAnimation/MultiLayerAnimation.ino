#include <Adafruit_NeoPixel.h>

#include <math.h>


// light strength 0 - 3
#define STRENGTH 3
#define EFFECTIVE_WIDTH 54

// Animations
#define MAX_ANIMATION 20


#define CHECK_FREQUENCY 1000


// LED positions
#define OUTDOOR 1

#define LINE_NUMBER 15
#define PIXELS_PER_LINE 60
#define PADDING 0

// 0 - 8. 0 means normal; +1 means double the animation speed.
#define FAST_FORWARD 1

uint16_t from_coordinate_to_number(byte x, byte y) {

  if ((OUTDOOR)) {
    //compensate the LEDs blocked by the window frames
    if (x >= 17 ) x += 3;
    if (x >= 40 ) x += 3;
  }


  if (x >= PIXELS_PER_LINE) return 0;
  if (y >= LINE_NUMBER) return 0;

  y = LINE_NUMBER - 1 - y; // start of control is from bottom
  uint16_t p;
  if (y % 2 != (OUTDOOR)) {
    x = PIXELS_PER_LINE - 1 - x;
  }
  p = y * (PIXELS_PER_LINE + PADDING) + x;

  return p;
};

class AnimationPlayer;

class Animation {
#define MAX_COLOR 16

    friend class AnimationPlayer;

  private:
    AnimationPlayer* player = NULL;

    byte color_bit_num = 1;


    // player control
    void setSerialNumber(AnimationPlayer* p, byte s) {
      player = p;
      serial_number = s;
    }


    // Canvas control: TODO separate into a class called Canvas

    byte maxColor() {
      byte max_color = 1 << color_bit_num ;
      if (max_color > MAX_COLOR) 
        return MAX_COLOR; 
      else 
        return max_color;
    }


    //color management
    uint32_t getColorFromCode(byte code) {
      if (code >= color_num) {
        Serial.print(code);
        Serial.print(" vs ");
        Serial.print(color_num);
        Serial.println(" getColorFromCode: code larger than color_num");
        Serial.flush();
        return 0;
      }
      return color_code[code];
    }

    byte getCodeFromColor(uint32_t color) {

      //TODO: potential optimization
      for (int i = 0; i < color_num; i++ ) {
        if (color_code[i] == color) return i;
      }

      if (color_num >= maxColor()) {
        for (int i = 0; i < color_num; i++ ) {
          Serial.print(color_code[i]);
          Serial.print(' ');
        }
        Serial.println(' ');
        Serial.println(color);
        Serial.print(serial_number);
        Serial.println(" getCodeFromColor: code larger than MAX");
        Serial.flush();
        return 0;
      }

      color_code[color_num] = color;
      color_num ++;
      return color_num - 1;
    }

    unsigned int getMatrixPos(byte x, byte y) {
      if ( x >= width || y >= height) {
        Serial.println("getMatrixPos: x or y out of bound");
        Serial.flush();
        return false;
      }      
      return ((unsigned int) y * width + x) * color_bit_num / 8;
    }

    byte getBitPos(byte x, byte y) {
      unsigned int pos = (unsigned int) y * width + x;
      return (pos % (8 / color_bit_num)) * color_bit_num;
      
    }
    byte getMask(byte x, byte y) {
      //TODO optimize
      byte mask_code = ((1 << color_bit_num ) - 1) << getBitPos(x, y);
      return (mask_code);
    }

    bool setMatrixColor(byte x, byte y, uint32_t color) {
      if ( x >= width || y >= height) {
        Serial.print(x);
        Serial.print(",");
        Serial.print(y);
        Serial.println(" setMatrixColor: x or y out of bound");
        Serial.flush();
        return false;
      }
      byte code = getCodeFromColor(color);
      unsigned int pos = getMatrixPos(x, y);   
      byte bitPos = getBitPos(x, y);
      byte mask = ~getMask(x, y);
      /*
      Serial.print(x); Serial.print(","); Serial.print(y); 
      Serial.print(" Color code: "); Serial.print(code); 
      Serial.print(" Pos: "); Serial.print(pos); 
      Serial.print(" BitPos: "); Serial.print(bitPos); 
      Serial.print(" BitMask: "); Serial.print(mask); 
      Serial.println(" "); Serial.flush();
      */
      matrix[pos] = (matrix[pos] & mask) | (code << bitPos);
      // Serial.println(matrix[pos]);Serial.flush();
      return true;
      
    }
    uint32_t getMatrixColor(byte x, byte y) {
        if ( x >= width || y >= height) {
          Serial.println("getMatrixColor x or y out of bound"); Serial.flush();
          return 0;
        }
        unsigned int pos = getMatrixPos(x, y);      
        byte bitPos = getBitPos(x, y);
        byte mask = getMask(x, y);      
        byte code = (matrix[pos] & mask) >> bitPos; 
        return getColorFromCode(code);
    }    

    byte* matrix;
    uint32_t* color_code;
    byte color_num;

  protected:
    byte serial_number = 0;
    byte min_y = 0, min_x = 0;
    byte width = 0, height = 0;
    unsigned long next_time = 0;

    bool moveAnimation(int dx, int dy);

    uint32_t getPixelColor(byte abs_x, byte abs_y) {
      if (abs_x < min_x || abs_y < min_y) return 0;
      if (abs_x >= (min_x + width) || abs_y >= (min_y + height)) return 0;
      return getMatrixColor(abs_x - min_x, abs_y - min_y);
    }

    bool setColor(byte x, byte y, uint32_t color);

    unsigned long refresh(unsigned long interval) {
      interval = interval >> FAST_FORWARD;
      next_time = millis() + interval;
    };

    void cleanCanvas() {
      color_num = 1;
      color_code[0] = 0;
      for (int i = 0; i < width; i ++) {
        for (int j = 0; j < height; j ++) {
          setColor(i, j, 0);
        }
      }
    }
    
    void dumpCanvas(); 

  public:
    // TODO: improve interface. we just need x and y; the width and height should be set by sub classes
    Animation(byte x1, byte y1, byte x2, byte y2, byte color_bit = 1) {

      if (x1 >= x2 || y1 >= y2 || x2 > EFFECTIVE_WIDTH || y2 > LINE_NUMBER) {
        Serial.println("Animation creation: x2 or y2 out of bound");
        Serial.flush();
        
      }

      color_bit_num = color_bit;

      min_x = x1;
      min_y = y1;
      
      width = x2 - min_x;
      height = y2 - min_y;


      color_code = new uint32_t[maxColor()];
      matrix = new byte[getMatrixPos(width - 1, height - 1) + 1];
      
      color_num = 1;
      color_code[0] = 0;
      
      for (int i = 0; i < (getMatrixPos(width - 1, height - 1) + 1); i ++) 
        matrix[i] = 0;

      Serial.print(width); Serial.print(" x "); Serial.print(height);
      Serial.print(" with ");Serial.print(maxColor());Serial.print(" colors and ");Serial.print(getMatrixPos(width - 1, height - 1) + 1);
      Serial.println(" matrix size, created");
      Serial.flush();      
      next_time = millis();      
    };
    ~Animation() {
      delete [] color_code;
      delete [] matrix;
    }
    unsigned long getShowTime() {
      return next_time;
    };
    static bool later(unsigned long a, unsigned long b) {
      // a is later than b
      return a > b;
      return ((a - b) >> 31) == 0; // wrap-around safe
    };
    virtual void render() {};
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

    byte layer[EFFECTIVE_WIDTH][LINE_NUMBER];

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
      Serial.println("initing player...");
      Serial.flush();
      blackout();
    };
    void setPlayDuration(unsigned long d = 0) {
      duration = d;
    };

    Adafruit_NeoPixel* getStrip() {
      return strip;
    };

    bool isOn() {
      return showing;
    };

    bool registerAnimation(Animation* a) {
      // true: successful
      // false: failed
      if (num >= MAX_ANIMATION) {
        Serial.println("AnimationPlayer: too many animations, cannot register new ones");
        Serial.flush();
        return false;
      }
      list[num] = a;
      a->setSerialNumber(this, num);
      num ++;
      return true;
    };

    bool setPixelColor(byte serial_number, byte x, byte y, uint32_t color) {
      // return if the color is changed

/*
        Serial.print(x);
        Serial.print(' ');
        Serial.print(y);
        Serial.print(' ');
        Serial.print(color);
        Serial.println(" Setting Pixel color");
        Serial.flush();
*/

      if (x >= EFFECTIVE_WIDTH && y >= LINE_NUMBER) {
        Serial.println("setPixelColor x or y out of bound");
        Serial.flush();
        return false;
      }

      if (layer[x][y] > serial_number) {
        /*
        Serial.print(layer[x][y]);
        Serial.print(" vs ");
        Serial.print(serial_number);
        Serial.println(" layer x,y is higher than serial number");
        Serial.flush();
        */
        return true; // no need to change if this pixel is covered by a higher layer.
      }

      while (color == 0 && serial_number > 0) {
        // giving up this layer of color
        serial_number --;
        color = list[serial_number]->getPixelColor(x, y);
      };

      layer[x][y] = serial_number;

      strip->setPixelColor(from_coordinate_to_number(x, y), color);

      /*
      // paint
        Serial.print(x);
        Serial.print(' ');
        Serial.print(y);
        Serial.print(' ');
        Serial.print(color);
        Serial.println(" Paint Pixel color to strip");
        Serial.flush();
      */
      return true;
    };


    unsigned long play() {
      // return the sleep time


      //Serial.print(num);
      //Serial.println(" animations play");
      //Serial.flush();

      if (num == 0) return CHECK_FREQUENCY;

      // next check
      if (!isOn()) {
        return CHECK_FREQUENCY;
      };
      unsigned long now = millis();
      if (duration) {
        //currently showing. and there is a fixed duration
        if (Animation::later(now, last_start + duration)) {
          turnOff();
          return CHECK_FREQUENCY;
        }
      }


      unsigned long ret = 0;
      while (ret == 0) {
        //Serial.println("   play() scan the list");
        //Serial.flush();
        unsigned long next = now + ((unsigned long)1 << 30); // set the original value of next to be the furthest away from now

        for (int i = 0; i < num; i ++) {
          if (Animation::later(now, list[i] -> getShowTime())) {
            list[i] -> render();
          }
          unsigned long local_next = list[i] -> getShowTime();
          if (Animation::later(next, local_next)) {
            next = local_next;
          }          
        }
        strip->show();
        now = millis();
        if (Animation::later(next, now)) ret = next - now; else ret = 0;
      }
      return ret;
    };

    void dumpLayer() {
      Serial.println("Dumping layer info:");
      for (int i = 0 ;i < LINE_NUMBER; i++) {
        for (int j = 0; j < EFFECTIVE_WIDTH; j++) {
          Serial.print(layer[j][i]);
          Serial.print(" ");
        }
        Serial.println(" ");
        Serial.flush();
      }
      Serial.println("Dumping layer info done");
      Serial.flush();
      
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
      //set all layer to be non-owner
      for (int i = 0; i < EFFECTIVE_WIDTH; i ++ ) {
        for (int j = 0; j < LINE_NUMBER; j ++) {
          layer[i][j] = 0;
        }
      }      
      for (uint16_t i = 0; i < strip->numPixels(); i++) {
        strip->setPixelColor(i, Adafruit_NeoPixel::Color(0,   0,   0));
      }
      strip->show();
    };

    void unitTest(int delta) {
      int led_number = PIXELS_PER_LINE * LINE_NUMBER;
      for (int i = 0; i < led_number; i += delta) {
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


bool Animation::setColor(byte x, byte y, uint32_t color) {
  if (x >= width && y >= height) {
    Serial.println("setColor x or y out of bound");  Serial.flush();
    return false;
  }
  if (!setMatrixColor(x, y, color)) {
    Serial.println("setColor setMatrixColor failed");  Serial.flush();
    return false;
  }
  if (!player || !player->setPixelColor(serial_number, x + min_x, y + min_y, color)) {
    Serial.println("setColor: setPixelColor failed");  Serial.flush();
    return false;
  }
  return true;
};


bool Animation::moveAnimation(int dx, int dy) {
  //Serial.println("moivng Animation");
  //Serial.flush();

  if ((min_x + dx + width) > EFFECTIVE_WIDTH 
  || (min_y + dy + height) > LINE_NUMBER
  || (min_x + dx) < 0
  || (min_y + dy) < 0
  ) {
    Serial.println("moveAnimation dx or dy out of bound");
    Serial.flush();
    return false;
  }

  // need to call AnimationPlayer to restore the space that is given up
  for (int j = min_y; j < min_y + height; j++) {
    if (dx > 0) {
      for (int i = min_x; i < min_x + dx; i ++) {      
        if (!player || !player->setPixelColor(serial_number, i, j, 0)) {
          Serial.println("moveAnimation: setPixelColor failed for dx>0 positions");  Serial.flush(); 
          return false;
        }
      }
    } else if (dx < 0) {
      for (int i = min_x + dx + width; i < min_x + width; i ++) {
        if (!player || !player->setPixelColor(serial_number, i, j, 0)) {
          Serial.println("moveAnimation: setPixelColor failed for dx<0 positions");  Serial.flush(); 
          return false;
        }
      }
    }
  }
  for (int i = min_x; i < min_x + width; i++) {
    if (dy > 0) {
      for (int j = min_y; j < min_y + dy; j ++) {      
        if (!player || !player->setPixelColor(serial_number, i, j, 0)) {
          Serial.println("moveAnimation: setPixelColor failed for dy>0 positions");  Serial.flush(); 
          return false;          
        }
      }
    } else if (dy < 0) {
      for (int j = min_y + dy + height; j < min_y + height; j ++) {
        if (!player || !player->setPixelColor(serial_number, i, j, 0)) {
          Serial.println("moveAnimation: setPixelColor failed for dy<0 positions");  Serial.flush(); 
          return false;                   
        }
      }
    }
  }  
  

  min_x += dx;
  min_y += dy;

  //call AnimationPlayer to refresh
  for (int i = 0; i < width; i ++ ) {
    for (int j = 0; j < height; j ++ ) {
      if (!player || !player->setPixelColor(serial_number, i + min_x, j + min_y, getMatrixColor(i, j))) {
        Serial.println("moveAnimation: setPixelColor failed for new positions");  Serial.flush();
        return false;
      }    
    }
  }


  return true;
};

void Animation::dumpCanvas() {
  Serial.print("Dumping Canvas. Serial number: ");
  Serial.print(serial_number);
  Serial.print(" Color: ");
  for (int i = 0; i < color_num; i++) {
    Serial.print(color_code[i]);
    Serial.print(" ");
  }
  Serial.print("color number: ");
  Serial.println(color_num);Serial.flush();

  Serial.print("Raw Matrix: ");
  unsigned int size_of_matrix= getMatrixPos(width - 1, height - 1) + 1;
  Serial.print(size_of_matrix);
  for (int i = 0; i < size_of_matrix; i++ ) {
    Serial.print(" ");
    Serial.print(matrix[i]);
  }      
  Serial.println("");
  Serial.flush();

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j ++) {
      Serial.print(getMatrixColor(j, i));
      Serial.print(" ");
    }
    Serial.println(" "); Serial.flush();
  }
  if (player) player->dumpLayer();
  Serial.println("Done with canvas dumping");

  Serial.flush();
};


class SnowAnimation: public Animation {
  private: 
    bool started = false;
  public:
    SnowAnimation(byte x1) : Animation(x1, 0, x1 + 1, 1) {};


    virtual void render() {
      if (!started) {
        if (rand() % 2 == 0) {
          setColor(0, 0, Adafruit_NeoPixel::Color(255,   255,   255));
          started = true;
        }
      } else if (min_y + 1 >= LINE_NUMBER) {
        cleanCanvas();
        min_x = rand() % (EFFECTIVE_WIDTH - 1);
        min_y = 0;
        started = false;
      } else {
        int dir = rand() % 3;

        if (dir == 1 && min_x < (EFFECTIVE_WIDTH - 1)) 
          moveAnimation(1, 1);
        else if (dir == 2 && min_x > 0) 
          moveAnimation(-1, 1);
        else 
          moveAnimation(0, 1);
      }
      refresh(1500);
    };

};



class BaloonAnimation: public  Animation {
  private:
    static byte tail;
    uint32_t color;
    bool started = false;
    byte last_tail = 1;
    const byte pixels[5][3] = {
      {0, 1, 0 },
      {1, 1, 1 },
      {0, 1, 0 },
      {0, 1, 0 },
      {0, 1, 0 },
    };


  public:

    BaloonAnimation(byte x_pos) : Animation(x_pos, 0, x_pos + 3, 5, 1) {

    };

    virtual void render() {
      if (!started) {
        if (rand() % 2 == 0) {
          cleanCanvas();
          switch (rand() % 4) {
            case 0:
              color = Adafruit_NeoPixel::Color(50 * STRENGTH, 17 * STRENGTH, 5 * STRENGTH); break;
            case 1:
              color = Adafruit_NeoPixel::Color(0, 0, 17 * STRENGTH); break;
            case 2:
              color = Adafruit_NeoPixel::Color(17 * STRENGTH, 17 * STRENGTH, 0); break;
            default:
              color = Adafruit_NeoPixel::Color(0, 17 * STRENGTH, 0);
          };          
          for (int i = 0; i < width; i ++ ) {
            for (int j = 0; j < height; j ++ ) {
              if (pixels[j][i]) {
                setColor(i, j, color);
              } else {
                setColor(i, j, 0);
              }              
            }
          }
          started = true;
        }
      } else if (min_y + height >= LINE_NUMBER) {
        cleanCanvas();
        min_x = rand() % (EFFECTIVE_WIDTH - width);
        min_y = 0;
        started = false;
      } else {
        int dir = rand() % 3;

        if (dir == 1 && min_x < (EFFECTIVE_WIDTH - width)) 
          moveAnimation(1, 1);
        else if (dir == 2 && min_x > 0) 
          moveAnimation(-1, 1);
        else 
          moveAnimation(0, 1);
      }


      // paint
      // set the tail string
      if (rand() % 3 == 0) {
        tail = rand() % 3;
      }
      if (tail != last_tail) {
        setColor(last_tail, 4, 0);
        last_tail = tail;
        setColor(last_tail, 4, color);        
      }

      refresh(7500);
    }

};

byte BaloonAnimation::tail = 1;

class PlaneAnimation: public  Animation {
  private:
    byte parity = 0;
    int dir = 1;
    bool started = false;

    byte pixels[5][6] = {
      {0, 2, 1, 0, 0, 0 },
      {0, 0, 0, 1, 0, 0 },
      {1, 3, 1, 1, 1, 3 },
      {0, 0, 0, 1, 0, 0 },
      {0, 4, 1, 0, 0, 0 },
    };

  public:
    PlaneAnimation(byte x1) : Animation(x1, 0, x1 + 6, 5, 3) {};


    virtual void render() {

      bool moved = false;
      if (parity % 4 == 0) {
        if (min_x == 0) {
          dir = 1;
        } else if (min_x + width >=  EFFECTIVE_WIDTH) {
          dir = -1;
        };
        moveAnimation(dir, 0);
        moved = true;
      }

      parity = (parity + 1) % 12;

      for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
          uint32_t color = 0;
          bool changed_color = moved;
          switch (pixels[dir > 0 ? y : height - 1 - y][dir > 0 ? x : width - 1 - x]) {
            case 0: color = 0; break;
            case 1: color = Adafruit_NeoPixel::Color(20, 20, 20); break; // fuselage
            case 4: color = Adafruit_NeoPixel::Color(255, 0, 0); break; // right wing light
            case 2: color = Adafruit_NeoPixel::Color(0, 255, 0); break; // left wing light
            case 3: color = (parity == 0 || parity == 3) ? Adafruit_NeoPixel::Color(255, 255, 255) : Adafruit_NeoPixel::Color(20, 20, 20); changed_color = true; break; // head light
            default: break;
          }
          if (changed_color) {
            setColor(x, y, color);
          }
        }
      refresh(100);

      /*
           if ((current_x > 1) && (current_x + 1) < content_width) {
             if (rand() % 3 == 0) {
               player->registerAnimation(new BaloonAnimation(current_x));
             }
           }
      */
      //if (current_x == 25) player->registerAnimation(new BaloonAnimation(current_x - content_width * dir));
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
          light_pixels_right[3 - i][j] = color ? 4 - color : 0;
        }
      }
    }
  public:

    CarAnimation(byte x1, byte y1, byte x2, byte y2) : Animation(x1, y1, x2, y2) {};

    virtual void render() {



      // Car
      for (int y = 0; y < content_height; y++)
        for (int x = 0; x < content_width; x++)
          setColor(x + current_x, y, 0);
      if (current_x <= 2) {
        dir = 1;
        changeLight(0);
      } else if (current_x + content_width >=  (width - 2)) {
        dir = -1;
        changeLight(2);
      } else {
        int predict_x = current_x;
        predict_x += dir * 10;
        if ( predict_x < 2 || (predict_x + content_width) > (width - 2) ) {
          changeLight(1);
        }
      }
      current_x += dir;


      //Lights
      for (int y = 0; y < light_height; y++)
        for (int x = 0; x < light_width; x++) {
          setColor(x, y, 0);
          setColor(width - 2 + x, y, 0);
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
          setColor(x, y, color);

          switch (light_pixels_right[y][x]) {
            case 0: color = 0; break;
            case 1: color = Adafruit_NeoPixel::Color(255, 0, 0); break; // Red Light
            case 2: color = Adafruit_NeoPixel::Color(255, 255, 0); break; // Yellow Light
            case 3: color = Adafruit_NeoPixel::Color(0, 255, 0);  break; // Green Light
            default: break;
          }
          setColor(width - 2 + x, y, color);
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
          setColor(x + current_x, y + 1, color);
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
        {0, 1, 0, 0, 0,},
        {1, 0, 1, 0, 0,},
        {1, 1, 1, 0, 0,},
        {1, 0, 1, 0, 0,},
        {1, 0, 1, 0, 0,},
      },
      { // B
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
      },

      { // C
        {0, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {0, 1, 1, 0, 0, },
      },
      { // D
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
      },
      { // E
        {1, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 1, 1, 0, 0, },
      },
      { // F
        {1, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
      },
      { // G
        {0, 1, 1, 1, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 1, 1, 1, },
        {1, 0, 0, 1, 0, },
        {0, 1, 1, 1, 0, },
      },
      { // H
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
      },
      { // I
        {1, 1, 1, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {1, 1, 1, 0, 0, },
      },
      { // J
        {1, 1, 1, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
      },
      { // K
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
      },
      { // L
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 1, 1, 0, 0, },
      },
      { // M
        {1, 0, 0, 0, 1,},
        {1, 1, 0, 1, 1,},
        {1, 0, 1, 0, 1,},
        {1, 0, 1, 0, 1,},
        {1, 0, 0, 0, 1,},
      },
      { // N
        {1, 0, 0, 1, 0, },
        {1, 1, 0, 1, 0, },
        {1, 0, 0, 1, 0, },
        {1, 0, 1, 1, 0, },
        {1, 0, 0, 1, 0, },
      },
      { // O
        {0, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {0, 1, 0, 0, 0, },
      },
      { // P
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
        {1, 0, 0, 0, 0, },
      },
      { // Q
        {0, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 0, 1, 0, 0, },
      },
      { // R
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
      },
      { // S
        {0, 1, 1, 0, 0, },
        {1, 0, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 0, 1, 0, 0, },
        {1, 1, 0, 0, 0, },
      },
      { // T
        {1, 1, 1, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
        {0, 1, 0, 0, 0, },
      },
      { // U
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 0, 1, 0, 0, },
        {1, 1, 1, 0, 0, },
      },
      { // V
        {1, 0, 1, 0, 0 },
        {1, 0, 1, 0, 0 },
        {1, 0, 1, 0, 0 },
        {0, 1, 0, 0, 0 },
        {0, 1, 0, 0, 0},
      },
      { // W
        {1, 0, 0, 0, 1 },
        {1, 0, 1, 0, 1 },
        {1, 0, 1, 0, 1 },
        {1, 1, 0, 1, 1 },
        {1, 0, 0, 0, 1 },
      },
      { // X
        {1, 0, 0, 1, 0 },
        {1, 0, 0, 1, 0 },
        {0, 1, 1, 0, 0 },
        {1, 0, 0, 1, 0 },
        {1, 0, 0, 1, 0 },
      },
      { // Y
        {1, 0, 1, 0, 0 },
        {1, 0, 1, 0, 0 },
        {0, 1, 0, 0, 0 },
        {0, 1, 0, 0, 0 },
        {0, 1, 0, 0, 0 },
      },
      { // Z
        {1, 1, 1, 0, 0 },
        {0, 0, 1, 0, 0 },
        {0, 1, 0, 0, 0 },
        {1, 0, 0, 0, 0 },
        {1, 1, 1, 0, 0 },
      },
    };
    const byte number_width[10] = {3, 1, 3, 3, 3, 3, 3, 3, 3, 3};
    const byte number[10][5][3] = {
      { // 0
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
      },
      { // 1
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
      },
      { // 2
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
      },
      { // 3
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
      },

      { // 4
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
      },
      { // 5
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
      },
      { // 6
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
      },
      { // 7
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
      },
      { // 8
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
      },
      { // 9
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
      },
    };

    const char* content = NULL;
    const char* content_list = NULL;

    byte pattern = 0;
    byte last_width = EFFECTIVE_WIDTH;




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
      last_width = width;
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
      //Serial.println("word rendering");
      //Serial.flush();
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

      cleanCanvas();

      const char* c = content;
      byte current_pos = 0;
      while ((*c) && (*c) != '\n') {
        if (current_pos) {
          // if this is not the first letter, add an empty column to separate letter
          for (int y = 0; y < 5; y++) {
            setColor(current_pos, y, 0);
          }
          current_pos++;
        }

        for (int y = 0; y < 5; y++)
          for (int x = 0; x < getLengthForCharacter(*c); x++)
            setColor(current_pos + x, y, getPixelsForCharactor(*c, x, y) ? color : 0);
        current_pos += getLengthForCharacter(*c);
        c ++;
      }

      if (current_pos < last_width) {
        // the last display is wider than this one, let's clean up the rest of the width
        for (int y = 0; y < height; y++)
          for (int x = current_pos; x < last_width; x ++) {
            setColor(x, y, 0);
          }
      }

      // Serial.println("word done with cleaning render");
      // Serial.flush();
      //dumpCanvas();


      // reset the width of last display
      last_width = current_pos;
      refresh(10000);

      //Serial.println("word done with render");
      //Serial.flush();
    }
};


class DayNightController {


#define LIGHT_PIN A7
#define LIGHT_THRESHOLD 512
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
  while (!Serial) ;


  Serial.println("start loop");
  Serial.flush();


  day_night_controller = new DayNightController();
  player = new AnimationPlayer();

  
  player->registerAnimation(new WordsAnimation(0, 6, EFFECTIVE_WIDTH, 11, "HAPPY HOLIDAYS\nHELLO 2019")); 


  player->registerAnimation(new PlaneAnimation(0));
// player->registerAnimation(new CarAnimation(0, 11, EFFECTIVE_WIDTH, 15));




  #define SNOW_NUM 6
  for (int i = 0; i < SNOW_NUM; i ++) {
    player->registerAnimation(new SnowAnimation(rand() % EFFECTIVE_WIDTH));
  }
  
  // R&D
  #define BALOON_NUM 3
  for (int i = 0; i < BALOON_NUM; i ++) {
    player->registerAnimation(new BaloonAnimation(rand() % (EFFECTIVE_WIDTH - 3)));
  }
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


  d = player->play();
  if (d) delay(d);
}



