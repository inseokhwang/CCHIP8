#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <GL/glut.h>

#define debug printf("debug\n");

// Display size
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define MODIFIER 10

// #define DRAWWITHTEXTURE

/*
 *      Variables for CPU
 */
FILE *rom;

unsigned short opcode;

unsigned char *reg;
unsigned short I;
unsigned short PC;

unsigned char *mem;

unsigned short *stack;
unsigned short SP;

unsigned char *gfx;
unsigned short drawFlag;

unsigned char sound_timer;
unsigned char delay_timer;

unsigned char *key;

void (*fp[16]) (unsigned short i);

unsigned char chip8_fontset[80] =
{ 
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

unsigned char getX(unsigned short i);
unsigned char getY(unsigned short i);
unsigned char getNN(unsigned short i);

/*
 *
 *     Functions for handling Opcodes and its own helper functions
 *
 *
 */

// Various shit for opcodes that start with 0
void fp0 (unsigned short i) {
        // Not worth simplifying through fp's to justify 3 opcodes.
        switch (i) {
                // Clear the screen TODO
                case 0x0E0:
                        for (int x = 0; x < 64; ++x) {
                                for (int y = 0; y < 32; ++y) {
                                        gfx[x + 64*y] = 0;
                                }
                        }
                        #ifdef DRAWWITHREFRESH
                        #else
                        drawFlag = 1;
                        #endif
                        break;
                // Return from subroutine
                case 0x0EE:
                        PC -= 2;
                        SP--;
                        PC = stack[SP];
                        break;
                // Calls RCA1802 program at address NNN - deprecated
                default: 
                        printf("Unknown opcode: 0%d\n", i);
                        exit(0);
                        break;
        }
}

// 1NNN - Jump to NNN
void fp1 (unsigned short i) {
        PC = i;
}

// 2NNN - Subroutine to NNN
void fp2 (unsigned short i) {
        stack[SP] = PC;
        SP++;
        PC = i;
}

// 3XNN - Skip the next instruction if Reg[X] = NN
void fp3 (unsigned short i) {
        unsigned char X = getX(i);
        if (reg[X] == getNN(i))
                PC += 2;
}

// 4XNN - Skip the next instruction if Reg[X] != NN
void fp4 (unsigned short i) {
        unsigned char X = getX(i);
        if (reg[X] != getNN(i))
                PC += 2;
}

// 5XY0 - Skip the next instruction if Reg[X] == Reg[Y]
void fp5 (unsigned short i) {
        unsigned char X = getX(i);
        unsigned char Y = getY(i);
        if (reg[X] == reg[Y])
                PC +=2;
}

// 6XNN - Move NN into Reg[X]
void fp6 (unsigned short i) {
        unsigned char X = getX(i);
        reg[X] = getNN(i);
}

// 7XNN - Adds NN to Reg[X]
void fp7 (unsigned short i) {
        unsigned char X = getX(i);
        reg[X] += getNN(i);
}

// TODO
void fp8 (unsigned short i) {
        unsigned char X = getX(i);
        unsigned char Y = getY(i);
        unsigned char N = i & 0xF;
        switch (N) {
                case 0x0:
                        reg[X] = reg[Y];
                        break;
                case 0x1:
                        reg[X] = reg[X] | reg[Y];
                        break;
                case 0x2:
                        reg[X] = reg[X] & reg[Y];
                        break;
                case 0x3:
                        reg[X] = reg[X] ^ reg[Y];
                        break;
                case 0x4:
                        if (((short) (reg[X] + reg[Y])) > 255)
                                reg[0xF] = 1;
                        else 
                                reg[0xF] = 0;
                        reg[X] += reg[Y];
                        break;
                case 0x5:
                        if (((short) (reg[X] - reg[Y])) < 0)
                                reg[0xF] = 0;
                        else
                                reg[0xF] = 1;
                        reg[X] -= reg[Y];
                        break;
                case 0x6:
                        reg[0xF] = reg[Y] & 0x1;
                        reg[X] = reg[Y] >> 1;
                        break;
                case 0x7:
                        if (((short) (reg[Y] - reg[X])) < 0)
                                reg[0xF] = 0;
                        else
                                reg[0xF] = 1;
                        reg[X] = reg[Y] - reg[X];
                        break;
                case 0xE:
                        reg[0xF] = (reg[Y] & 0xA0) >> 7;
                        reg[X] = reg[Y] << 1;
        }
}

// 9XY0 - Skips the next instruction if Reg[X] != Reg[Y]
void fp9 (unsigned short i) {
        unsigned char X = getX(i);
        unsigned char Y = getY(i);
        if (reg[X] != reg[Y])
                PC +=2;
}

// ANNN - Sets I to NNN
void fpA (unsigned short i) {
        I = i;
}

// BNNN - Jumpts to NNN + Reg[0] - WARNING might add into 0 I think, should use mod here?
void fpB (unsigned short i) {
        PC = i + reg[0x0];
        PC -= 2;
}

// CXNN - Sets Reg[X] as a random number between 0 and NN
void fpC (unsigned short i) {
        unsigned char X = getX(i);
        reg[X] = (rand() % 0xFF) & getNN(i);
}

// DXYN - Draw a sprite at (Reg[X], Reg[Y]) that is N bytes long and is located at the Index register
void fpD (unsigned short i) {
        unsigned char X = getX(i);
        unsigned char Y = getY(i);
        X = reg[X];
        Y = reg[Y];
        unsigned char N = i & 0xF;
        reg[0xF] = 0;
        unsigned char pixel;
        for (char y = 0; y < N; ++y) {
                pixel = mem[I + y];
                for (char x = 0; x < 8; ++x) {
                        int pixelIndex = X + x + 64*(Y + y);
                        if (X + x >= 64)
                                pixelIndex -= 64;
                        if (Y + y >= 32)
                                pixelIndex -= 64*32;
                        if (X + x < 0)
                                pixelIndex += 64;
                        if (Y + y < 0)
                                pixelIndex += 64*32;
                        if (pixel & (0x80 >> x)) {
                                if (gfx[pixelIndex]) {
                                        reg[0xF] = 1;
                                }
                                gfx[pixelIndex] ^= 1;
                        }
                }
        }
        #ifdef DRAWWITHREFRESH
        #else
        drawFlag = 1;
        #endif
}

/* 
 * EX9E - Skips the next instruction if the key stored in VX is pressed
 * EXA1 - Skips the next isntruction if the key stored in VX isn't pressed
 */

void fpE (unsigned short i) {
        unsigned char X = getX(i);
        X = reg[X];
        char pressed = key[X];
        switch (getNN(i)) {
                case 0x9E:
                        if (pressed)
                                PC += 2;
                        break;
                case 0xA1:
                        if (!pressed)
                                PC += 2;
                        break;
        }
}

// Various opcodes that start with F
void fpF (unsigned short i) {
        unsigned char X = getX(i);
        unsigned char NN = getNN(i);
        char pressed;
        switch (NN) {
                // Store delay_timer in reg[X]
                case 0x07:
                        reg[X] = delay_timer;
                        break;
                // Wait for a keypress
                case 0x0A:
                        pressed = 0;
                        for (int z = 0; z < 16; ++z) {
                                pressed += key[z];
                        }
                        if (pressed)
                                PC -= 2;
                // Set delay_timer to reg[X]
                case 0x15:
                        delay_timer = reg[X];
                        break;
                // Set the sound_timer to reg[X]
                case 0x18:
                        sound_timer = reg[X];
                        break;
                // Set I = I + reg[X]
                case 0x1E:
                        if (((short) I + reg[X]) > 255)
                                reg[0xF] = 1;
                        else
                                reg[0xF] = 0;
                        I += reg[X];
                        break;
                // Set I to correct font specified by reg[X] 
                case 0x29:
                        I = reg[X] * 5;
                        break;
                // Write a decimal representation of reg[X] into I, I+1, I+2
                case 0x33:
                        X = reg[X];
                        mem[I] = X / 100;
                        mem[I + 1] = (X / 10) % 10;
                        mem[I + 2] = X % 10;
                        break;
                case 0x55:
                        for (int z = 0; z <= X; z++) {
                                mem[I + z] = reg[z];
                        }
                        I = I + X + 1;
                        break;
                case 0x65:
                        for (int z = 0; z <= X; z++) {
                                reg[z] = mem[I + z];
                        }
                        I = I + X + 1;
                        break;
        }

}

unsigned char getX (unsigned short i) {
        unsigned char X = (i & 0xF00) >> 8;
        return X;
}

unsigned char getY (unsigned short i) {
        unsigned char Y = (i & 0xF0) >> 4;
        return Y;
}

unsigned char getNN (unsigned short i) {
        return i & 0xFF;
}

/*
 *      Variables for GUI
 */

int display_width = SCREEN_WIDTH * MODIFIER;
int display_height = SCREEN_HEIGHT * MODIFIER;
typedef unsigned char u8;
u8 screenData [SCREEN_HEIGHT] [SCREEN_WIDTH] [3];

void reshape_window(GLsizei w, GLsizei h);
void runCycle();
void setInput();

/*
 *
 *      Methods to help set up the graphics
 *
 */

void initTexture() {
        // Clear screen
        for(int y = 0; y < SCREEN_HEIGHT; ++y)              
                for(int x = 0; x < SCREEN_WIDTH; ++x)
                        screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;

        // Create a texture 
        glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

        // Set up the texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 

        // Enable textures
        glEnable(GL_TEXTURE_2D);
}

void drawPixel(int x, int y)
{
        glBegin(GL_QUADS);
                glVertex3f((x * MODIFIER) + 0.0f,     (y * MODIFIER) + 0.0f,     0.0f);
                glVertex3f((x * MODIFIER) + 0.0f,     (y * MODIFIER) + MODIFIER, 0.0f);
                glVertex3f((x * MODIFIER) + MODIFIER, (y * MODIFIER) + MODIFIER, 0.0f);
                glVertex3f((x * MODIFIER) + MODIFIER, (y * MODIFIER) + 0.0f,     0.0f);
        glEnd();
}

void updateTexture() {
        for(int y = 0; y < 32; ++y)            
                for(int x = 0; x < 64; ++x)
                        if(gfx[(y * 64) + x] == 0)
                                screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;    // Disabled
                        else 
                                screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 255;  // Enabled

        // Update Texture
        glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

        glBegin( GL_QUADS );
                glTexCoord2d(0.0, 0.0);         glVertex2d(0.0, 0.0);
                glTexCoord2d(1.0, 0.0);         glVertex2d(display_width, 0.0);
                glTexCoord2d(1.0, 1.0);         glVertex2d(display_width, display_height);
                glTexCoord2d(0.0, 1.0);         glVertex2d(0.0, display_height);
        glEnd();
 }

void updateQuads()
{
        // Draw
        for(int y = 0; y < 32; ++y)             
                for(int x = 0; x < 64; ++x)
                {
                        if(gfx[(y*64) + x] == 0) 
                                glColor3f(0.0f,0.0f,0.0f);                      
                        else 
                                glColor3f(1.0f,1.0f,1.0f);

                        drawPixel(x, y);
                }
}

void drawScreen() {
        runCycle();
        if(drawFlag) {
                glClear(GL_COLOR_BUFFER_BIT);

#ifdef DRAWWITHTEXTURE
                updateTexture();
#else
                updateQuads();
#endif
                glutSwapBuffers();

                drawFlag = 0;
        }
}

void reshape_window(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
 
    // Resize quad
    display_width = w;
    display_height = h;
}

void keyboardDown(unsigned char k, int x, int y)
{
        if(k == 27)    // esc
                exit(0);

        else if(k == '1')  key[0x1] = 1;
        else if(k == '2')  key[0x2] = 1;
        else if(k == '3')  key[0x3] = 1;
        else if(k == '4')  key[0xC] = 1;

        else if(k == 'q')  key[0x4] = 1;
        else if(k == 'w')  key[0x5] = 1;
        else if(k == 'e')  key[0x6] = 1;
        else if(k == 'r')  key[0xD] = 1;

        else if(k == 'a')  key[0x7] = 1;
        else if(k == 's')  key[0x8] = 1;
        else if(k == 'd')  key[0x9] = 1;
        else if(k == 'f')  key[0xE] = 1;

        else if(k == 'z')  key[0xA] = 1;
        else if(k == 'x')  key[0x0] = 1;
        else if(k == 'c')  key[0xB] = 1;
        else if(k == 'v')  key[0xF] = 1;
}

void keyboardUp(unsigned char k, int x, int y)
{
        if(k == '1')  key[0x1] = 0;
        else if(k == '2')  key[0x2] = 0;
        else if(k == '3')  key[0x3] = 0;
        else if(k == '4')  key[0xC] = 0;

        else if(k == 'q')  key[0x4] = 0;
        else if(k == 'w')  key[0x5] = 0;
        else if(k == 'e')  key[0x6] = 0;
        else if(k == 'r')  key[0xD] = 0;

        else if(k == 'a')  key[0x7] = 0;
        else if(k == 's')  key[0x8] = 0;
        else if(k == 'd')  key[0x9] = 0;
        else if(k == 'f')  key[0xE] = 0;

        else if(k == 'z')  key[0xA] = 0;
        else if(k == 'x')  key[0x0] = 0;
        else if(k == 'c')  key[0xB] = 0;
        else if(k == 'v')  key[0xF] = 0;
}


/*
 *
 *      Main methods used for emulation
 *
 */


//Initializes registers and memory
void initialize() {
        PC = 0x200;
        opcode = 0;
        I = 0;
        SP = 0;

        reg = calloc(16 , sizeof(char));
        mem = calloc(4096 , sizeof(char));
        stack = calloc(16 , sizeof(short));
        gfx = calloc(64 * 32, sizeof(char));
        key = calloc(16 , sizeof(char));

        //Set up font
        for (int i = 0; i < 80; i++) {
                mem[i] = chip8_fontset[i];
        }

        drawFlag = 1;
        delay_timer = 0;
        sound_timer = 0;

        //Set up random seed for rand
        srand (time(0));

        //Set up function pointers for opcodes
        fp[0x0] = fp0;
        fp[0x1] = fp1;
        fp[0x2] = fp2;
        fp[0x3] = fp3;
        fp[0x4] = fp4;
        fp[0x5] = fp5;
        fp[0x6] = fp6;
        fp[0x7] = fp7;
        fp[0x8] = fp8;
        fp[0x9] = fp9;
        fp[0xA] = fpA;
        fp[0xB] = fpB;
        fp[0xC] = fpC;
        fp[0xD] = fpD;
        fp[0xE] = fpE;
        fp[0xF] = fpF;

}

void loadGame(char* gameDirectory) {
        rom = fopen(gameDirectory, "rb");
        if (rom == 0) {
                printf("Error opening file");
                exit(0);
        } 
        fseek(rom, 0, SEEK_END);
        long size = ftell(rom);
        rewind(rom);
        unsigned char *buffer = malloc(size * sizeof(char));
        if (size != fread(buffer, 1, size, rom)) {
                printf("Error reading file");
                exit(0);
        }
        for(int i = 0; i < size; ++i) {
                mem[PC + i] = buffer[i];
        }
        
        fclose(rom);
        free(buffer);
}

void printHelp() {
        printf("\nUsage: chip8 gamefile");
}

// Fetch, Decode, Execute Opcode and Update timer
void runCycle() {
        opcode = mem[PC] << 8 | mem[PC + 1];
        unsigned short i = opcode & 0xFFF;
        opcode = opcode >> 12;
        PC += 2;
        (*fp[opcode]) (i);
        if (sound_timer > 0) {
                if (sound_timer == 1) {
                        //TODO make a sound?
                }
                --sound_timer;
        }
        if (delay_timer > 0)
                --delay_timer;
}

// Text based display for debugging purposes
void debugDisplay() {
        for(int y = 0; y < 32; ++y)
        {
                for(int x = 0; x < 64; ++x)
                {
                        if(gfx[(y*64) + x] == 0) 
                                printf("O");
                        else 
                                printf(" ");
                }
                printf("\n");
        }
        printf("\n");
}

int main(int argc, char *argv[]) {
        initialize();
        if (argc > 0) {
                loadGame(*(argv + 1));
        } else {
                printHelp();
        }

        // while (1) {
        //         runCycle();
        //         if (drawFlag) {
        //                 debugDisplay();
        //                 drawFlag = 0;
        //         }
        // }

        glutInit(&argc, argv);          
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

        glutInitWindowSize(display_width, display_height);
        glutInitWindowPosition(0, 0);
        glutCreateWindow("CCHIP-8");

        glutDisplayFunc(drawScreen);
        glutIdleFunc(drawScreen);
        glutReshapeFunc(reshape_window);
        glutKeyboardFunc(keyboardDown);
        glutKeyboardUpFunc(keyboardUp); 

        updateTexture();

        glutMainLoop(); 

        return 0;
}