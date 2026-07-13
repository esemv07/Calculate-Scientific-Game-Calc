#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <math.h>


/*
-- PROJECT FLOW --

setup:
- setup IO expander
- setup screen
- display initial screen

loop: 
- check for button press
- draw display - could probably leave this out of loop because it reloads on every input

check for button press:
- check whether shifted
- return row + column with shifted/normal key

HANDLE KEY PRESS
Types of key presses:
- Input keys
  - number (0-9)
  - in-built operation (+ - / * ^)
  - special operation (! sqrt otherroot x10^ %)
  - trig function (sin cos tan asin acos atan)
  - other function (log ln Abs nPr nCr)
  - constants (Pi Euler Ans)
  - other inputs ('(' ')' ',' '.')
- Control keys
  - other (Mode Game Shift)
  - deletion (AC DEL)
  - equals (= S<=>D Deg/Min/Sec)
  - direction (UP DOWN LEFT RIGHT)

Flow:
if input key : append to expression

if control key : check - 'can control be executed before evaluation'
  if yes : execute
  if no : continue


EVALUATION
- separate expression into order of operations
  1. brackets
  2. functions
  3. exponents
  4. operations (* /)
  5. operations (+ -)
- evaluate in separated parts
- return ans
- store ans as Ans
- draw display

Flow:
- convert expression to tokens
- turn into RPN
- evaluate RPN
- store Ans
- return ans to display

*/


#define TFT_CS  10
#define TFT_RST 8
#define TFT_DC 9

#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 128

// No. of characters that fit on each line (may be changed once I test with actual parts)
// #define CHARS_PER_LINE 26

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

Adafruit_MCP23X17 mcp;

const int ROWS = 8;
const int COLS = 5;

const int rowPins[ROWS] = {0, 1, 2, 3, 4, 5, 6, 7}; // GPA0-GPA7
const int colPins[COLS] = {8, 9, 10, 11, 12}; // GPB0-GPB4

bool isShifted = false;
bool degreeMode = false;
String expression = "";
String errorMessage = "";
double ans = 0;
String ansString = "";
bool clearOnNextKey = false;
int cursorPosition;
double previousAns = 0;


char keymapNormal[ROWS][COLS] = {
  {'S', 'L', 'U', 'R', 'M'}, // Shift, Left, Up, Right, Mode
  {'q', '^', 'D', 'g', '!'}, // Sqrt, x^y, Down, Log10(), Factorial
  {'f', 'd', 's', 'c', 't'}, // Fraction, Degrees/Mins/Secs, Sin, Cos, Tan
  {'e', 'p', '(', ')', '#'}, // Euler's Number, Pi, (, ), S<=>D
  {'7', '8', '9', 'X', 'C'}, // 7, 8, 9, DEL, AC
  {'4', '5', '6', '*', '/'}, // 4, 5, 6, Multiply, Divide
  {'1', '2', '3', '+', '-'}, // 1, 2, 3, Add, Subtract
  {'0', '.', 'x', 'A', '='}  // 0, ., x10^y, Ans, =
};

char keymapShift[ROWS][COLS] = {
  {'S', 'L', 'U', 'R', 'G'}, // Shift, Left, Up, Right, Game
  {'r', '^', 'D', 'l', '!'}, // Root, x^y, Down, Ln(), Factorial
  {'|', 'F', 'z', 'O', 'T'}, // Absolute Value, Factors, Sin-1, Cos-1, Tan-1
  {'e', 'p', '(', ')', '#'}, // Euler's Number, Pi, %, ,, S<=>D
  {'7', '8', '9', 'X', 'C'}, // 7, 8, 9, DEL, AC
  {'4', '5', '6', 'P', 'N'}, // 4, 5, 6, nPr, nCr
  {'1', '2', '3', '+', '-'}, // 1, 2, 3, Add, Subtract
  {'0', '.', 'x', 'A', '='}  // 0, ., x10^y, Ans, =
};

enum TokenType {
  NUMBER,
  OPERATOR,
  FUNCTION,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  POSTFIX,
  SCIENTIFIC
};

struct Token {
  TokenType type;
  char text[8];
  double value;
};

Token tokens[32];
int tokenCount = 0;

Token output[32];
Token opStack[32];

int outputCount = 0;
int opTop = -1;

double valueStack[32];
int valueTop = -1;


void setup() {
  // put your setup code here, to run once:
  Wire.begin();

  tft.initR(INITR_MINI_DEFAULT);

  tft.invertDisplay(true);

  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);

  if (!mcp.begin_I2C(0x20)) {
    Serial.println("MCP23017 not detected");
    while(1);
  };

  for (byte r = 0; r < ROWS; r++) {
    mcp.pinMode(rowPins[r], OUTPUT);
    mcp.digitalWrite(rowPins[r], HIGH);
  }

  for (byte c = 0; c < COLS; c++) {
    mcp.pinMode(colPins[c], INPUT_PULLUP);
  }

  drawDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:
  char key = scanMatrix();
  if (key != '\0') {
    handleKeyPress(key);
    delay(250);
  }
}


char scanMatrix() {
  for (byte r = 0; r < ROWS; r++) {
    mcp.digitalWrite(rowPins[r], LOW);

    for (byte c = 0; c < COLS; c++) {
      if (mcp.digitalRead(colPins[c]) == LOW) {
        while(mcp.digitalRead(colPins[c]) == LOW);

        mcp.digitalWrite(rowPins[r], HIGH);

        char key;

        if (isShifted) {
          key = keymapShift[r][c];
          return key;
        } else {
          key = keymapNormal[r][c];
          return key;
        }
      }
    }

    mcp.digitalWrite(rowPins[r], HIGH);
  }
  return '\0';
}


void handleKeyPress(char key) {

  // Shift
  if (key == 'S') {
    isShifted = !isShifted;
    return;
  }

  if (clearOnNextKey && ((key >= '0' && key <= '9') || key == '.')) {
    expression = "";
    ansString = "";
    clearOnNextKey = false;
  }

  // Numeric Appends
  else if (isDigit(key) || key == '.') {
    insertCharacter(key);
  }

  // Basic Operations
  else if (key == '+' || key == '-' || key == '*' || key == '/' || key == '^') {
    insertCharacter(key);
    return;
  }

  else if (key == '(' || key == ')') {
    insertCharacter(key);
  }

  // Constants
  else if (key == 'p') {
    insertCharacter('π');
  }

  else if (key == 'e') {
    insertCharacter('e');
  }

  else if (key == 'A') {
    insertAns();
  }

  // Advanced Functions
  else if (key == 's') {
    insertFunction("sin(");
  }
  else if (key == 'z') {
    insertFunction("asin(");
  }
  else if (key == 'c') {
    insertFunction("cos(");
  }
  else if (key == 'O') {
    insertFunction("acos(");
  }
  else if (key == 't') {
    insertFunction("tan(");
  }
  else if (key == 'T') {
    insertFunction("atan(");
  }

  else if (key == 'q') {
    insertFunction("sqrt(");
  }

  else if (key == 'g') {
    insertFunction("log(");
  }

  else if (key == 'l') {
    insertFunction("ln(");
  }

  else if (key == 'r') {
    insertCharacter('r');
  }

  else if (key == 'P') {
    insertCharacter("P");
  }

  else if (key == 'N') {
    insertCharacter("C");
  }

  else if (key == 'x') {
    insertFunction("x10^");
  }

  // Cursor
  else if (key == 'L') {
    if (cursorPosition > 0) {
      cursorPosition--;
    } else {
      cursorPosition = expression.length();
    }
  }

  else if (key == 'R') {
    if (cursorPosition < expression.length()) {
      cursorPosition++;
    } else {
      cursorPosition = 0;
    }
  }

  else if (key == 'U') {
    moveCursorUp();
  }

  else if (key == 'D') {
    moveCursorDown();
  }

  // Delete and AC
  else if (key == 'X') {
    if (cursorPosition > 0) {
      expression.remove(cursorPosition - 1, 1);
      cursorPosition--;
    }
  }

  else if (key == 'C') {
    expression = "";
    cursorPosition = 0;
    ans = 0;
    errorMessage = "";
  }

  // Mode 
  else if (key == 'M') {
    degreeMode = !degreeMode;
  }

  // Equals
  if (key == '=') {
    if (expression.length() > 0) {
      tokenize(expression);
      shuntingYard(tokens, tokenCount);
      ans = evaluateRPN()
      ansString = String(ans);
      previousAns = ans;
      cursorPosition = expression.length();
      drawAns();
    }
    return;
  }

  if (isShifted) {
    isShifted = false;
  }

  drawDisplay();
}

void insertCharacter(char c) {
  expression = expression.substring(0, cursorPosition) + c + expression.substring(cursorPosition);
  cursorPosition++;
}

void insertFunction(String functionName) {
  String text = functionName;

  expression = expression.substring(0, cursorPosition) + text + expression.substring(cursorPosition);
  cursorPosition += text.length();
}

void insertAns() {
  expression = expression.substring(0, cursorPosition) + "Ans" + expression.substring(cursorPosition);
}

void moveCursorUp() {
  
}

void moveCursorDown() {
  
}

void drawDisplay() {
  tft.fillScreen(ST77XX_BLACK);
  drawStatusBar();
  drawExpression();
  drawCursor();

  if (errorMessage.length() > 0) {
    tft.setCursor(100, 100);
    tft.print(errorMessage);
  }
}

void drawAns() {
  tft.setCursor(100, 100);
  tft.print(ansString);
}

void drawStatusBar() {
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  if (isShifted) {
    tft.print("SHIFT ");
  } else {
    tft.print("      ");
  }

  if (degreeMode) {
    tft.print("DEG");
  } else {
    tft.print("RAD");
  }
}

void drawExpression() {
  tft.setCursor(0, 20);
  tft.print(expression);
}

void drawCursor() {
  tft.setCursor(cursorPosition * 6, 32);
  tft.print("|");
}


void tokenize(const String &expression) {
  tokenCount = 0;
  i = 0;

  while (i < expression.length()) {
    char c = expression[i];
    if (c == ' ') {
      i++;
      continue;
    }

    if (isDigit(c) || c == '.') {
      String number = "";
      while (i < expression.length() && (isDigit(c) || expression[i] == '.')) {
        number += expression[i];
        i++;
      }

      tokens[tokenCount].type = NUMBER;
      tokens[tokenCount].value = number.toFloat();
      tokenCount++;

      continue;
    }

    if (c == 'π') {
      tokens[tokenCount].type = NUMBER;
      tokens[tokenCount].value = PI;
      tokenCount++;
      i++;
      continue;
    }
    if (c == 'e') {
      tokens[tokenCount].type = NUMBER;
      tokens[tokenCount].value = exp(1.0);
      tokenCount++;
      i++;
      continue;
    }
    if (expression.startsWith("Ans", i)) {
      tokens[tokenCount].type = NUMBER;
      tokens[tokenCount].value = previousAns;
      tokenCount++;
      i += 3;
      continue;
    }

    if (expression.startsWith("sqrt", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "sqrt");
      tokenCount++;
      i += 4;
      continue;
    }

    if (expression.startsWith("sin", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "sin");
      tokenCount++;
      i += 3;
      continue;
    }
    if (expression.startsWith("cos", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "cos");
      tokenCount++;
      i += 3;
      continue;
    }
    if (expression.startsWith("tan", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "tan");
      tokenCount++;
      i += 3;
      continue;
    }
    if (expression.startsWith("asin", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "asin");
      tokenCount++;
      i += 4;
      continue;
    }
    if (expression.startsWith("acos", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "acos");
      tokenCount++;
      i += 4;
      continue;
    }
    if (expression.startsWith("atan", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "atan");
      tokenCount++;
      i += 4;
      continue;
    }
    if (expression.startsWith("ln", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "ln");
      tokenCount++;
      i += 2;
      continue;
    }
    if (expression.startsWith("log", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "log");
      tokenCount++;
      i += 3;
      continue;
    }
    if (expression.startsWith("abs", i)) {
      tokens[tokenCount].type = FUNCTION;
      strcpy(tokens[tokenCount].text, "abs");
      tokenCount++;
      i += 3;
      continue;
    }

    if (expression.startsWith("x10^", i)) {
      tokens[tokenCount].type = SCIENTIFIC;
      strcpy(tokens[tokenCount].text, "x10^");
      tokenCount++;
      i += 4;
      continue;
    }

    if (c == '(') {
      tokens[tokenCount].type = LEFT_BRACKET;
      tokens[tokenCount].text[0] = c;
      tokens[tokenCount].text[1] = '\0';
      tokenCount++;
      i++;
      continue;
    }
    if (c == ')') {
      tokens[tokenCount].type = RIGHT_BRACKET;
      tokens[tokenCount].text[0] = c;
      tokens[tokenCount].text[1] = '\0';
      tokenCount++;
      i++;
      continue;
    }

    if (strchr("!", c)) {
      tokens[tokenCount].type = POSTFIX;
      tokens[tokenCount].text[0] = c;
      tokens[tokenCount].text[1] = '\0';
      tokenCount++;
      i++;
      continue;
    }

    if (strchr("+-*/PC^r", c)) {
      tokens[tokenCount].type = OPERATOR;
      tokens[tokenCount].text[0] = c;
      tokens[tokenCount].text[1] = '\0';
      tokenCount++;
      i++;
      continue;
    }
  }
}

int precedence(char operation) {
  switch operation {
    case '+':
    case '-':
      return 1;
    
    case '*':
    case '/':
    case 'P':
    case 'C':
      return 2;
    
    case '^':
    case 'r':
      return 3;
    
    default:
      return 0;
  }
} 

void shuntingYard(Token tokens[], int length) {
  outputCount = 0;
  opTop = -1;

  for (int i = 0; i < length; i++) {
    Token token = tokens[i];

    if (token.type == NUMBER) {
      output[outputCount] = token;
    }

    else if (token.type == FUNCTION) {
      pushOperator(token.text);
    }

    else if (token.type == OPERATOR) {
      while (!operatorEmpty()) {
        Token top = peekOperator();

        if (top.text[0] == '(') {
          break;
        }

        if (precedence(top) > precedence(token.text) || (precedence(top) == precedence(token.text) && !rightAssociative(token.text))) {
          Token temp;
          temp.type = OPERATOR;
          strcpy(temp.text, popOperator().text);

          output[outputCount++] = temp;
        } else {
          break;
        }
      }
      pushOperator(token.text);
    }

    else if (token.type == LEFT_BRACKET) {
      pushOperator('(');
    }

    else if (token.type == RIGHT_BRACKET) {
      while (!operatorEmpty() && strcmp(peekOperator().text, "(") != 0) {
        Token temp;
        temp.type = OPERATOR;
        strcpy(temp.text, popOperator().text);

        output[outputCount++] = temp;
      }

      if (!operatorEmpty()) {
        popOperator();
      }
      if (!operatorEmpty()) {
        Token top = peekOperator();

        if (top.type == FUNCTION) {
          Token temp;
          temp.type = FUNCTION;
          strcpy(temp.text, popOperator().text);

          output[outputCount++] = temp;
        }
      }
    }

    else if (token.type == POSTFIX) {
      output[outputCount++] = token;
    }
  }

  while (!operatorEmpty()) {
    Token temp;
    temp.type = OPERATOR;
    strcpy(temp.text, popOperator().text);

    output[outputCount++] = temp;
  }

}

bool rightAssociative(char operation) {
  if (operation == '^') {
    return true;
  }
  return false;
}

void pushOperator(String operation) {
  opStack[++opTop] = operation;
}

Token popOperator() {
  return opStack[opTop--];
}

Token peekOperator() {
  return opStack[opTop];
}

bool operatorEmpty() {
  return opTop < 0;
}


double evaluateRPN() {
  for (int i = 0; i < outputCount; i++) {
    Token token = output[i];

    if (token.type == NUMBER) {
      pushValue(token.value);
    }

    else if (token.type == OPERATOR) {
      double b = popValue();
      double a = popValue();

      if (strcmp(token.text, "+") == 0) {
        pushValue(a + b);
      } else if (strcmp(token.text, "-") == 0) {
        pushValue(a - b);
      } else if (strcmp(token.text, "*") == 0) {
        pushValue(a * b);
      } else if (strcmp(token.text, "/") == 0) {
        pushValue(a / b);
      } else if (strcmp(token.text, "^") == 0) {
        pushValue(pow(a, b));
      } else if (strcmp(token.text, "P") == 0) {
        pushValue(nPr(a, b));
      } else if (strcmp(token.text, "C") == 0) {
        pushValue(nCr(a, b));
      } else if (strcmp(token.text, "r") == 0) {
        pushValue(pow(b, (1.0 / a)));
      }
    }

    else if (token.type == FUNCTION) {
      double a = popValue();
      
      if (!degreeMode) {
        if (strcmp(token.text, "sin") == 0) {
          pushValue(sin(a));
        } else if (strcmp(token.text, "cos") == 0) {
          pushValue(cos(a));
        } else if (strcmp(token.text, "tan") == 0) {
          pushValue(tan(a));
        } else if (strcmp(token.text, "asin") == 0) {
          pushValue(asin(a));
        } else if (strcmp(token.text, "acos") == 0) {
          pushValue(acos(a));
        } else if (strcmp(token.text, "atan") == 0) {
          pushValue(atan(a));
        } 
      } else {
        double b = a * (PI / 180.0);
        if (strcmp(token.text, "sin") == 0) {
          pushValue(sin(b));
        } else if (strcmp(token.text, "cos") == 0) {
          pushValue(cos(b));
        } else if (strcmp(token.text, "tan") == 0) {
          pushValue(tan(b));
        } else if (strcmp(token.text, "asin") == 0) {
          pushValue(asin(a) * (180.0 / PI));
        } else if (strcmp(token.text, "acos") == 0) {
          pushValue(acos(a) * (180.0 / PI));
        } else if (strcmp(token.text, "atan") == 0) {
          pushValue(atan(a) * (180.0 / PI));
        }
      }

      if (strcmp(token.text, "sqrt") == 0) {
        pushValue(sqrt(a));
      } else if (strcmp(token.text, "log") == 0) {
        pushValue(log10(a));
      } else if (strcmp(token.text, "ln") == 0) {
        pushValue(log(a));
      } else if (strcmp(token.text, "abs") == 0) {
        pushValue(fabs(a));
      }
    }

    else if (token.type == POSTFIX) {
      double a = popValue();

      if (strcmp(token.text, "!") == 0) {
        pushValue(factorial(a));
      }
    }

    else if (token.type == SCIENTIFIC) {
      double a = popValue();
      double b = popValue();

      pushValue(b * pow(10, a));
    }
  }

  return popValue();
}

void pushValue(double x) {
  valueStack[++valueTop] = x;
}

double popValue() {
  return valueStack[valueTop--];
}


unsigned long factorial(int n) {
  unsigned long result = 1;
  for (int i = 2; i <= n; i++) {
    result *= i;
  }

  return result;
}

double nPr(double n, double r) {
  int N = (int)n;
  int R = (int)r;
  return factorial(N) / factorial(N - R);
}

double nCr(double n, double r) {
  int N = (int)n;
  int R = (int)r;
  return factorial(N) / (factorial(R) * factorial(N - R));
}
