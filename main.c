#include <stdio.h>          // Standard input output library
#include <SDL2/SDL.h>       // SDL2 for graphical window
                            
#define CAM_WIDTH 1280      
#define CAM_HEIGHT 840   

#define NUM_BUFFERS 2
#define UPDATE_TIME .01


typedef struct Pixel
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;
} Pixel;

void **bufferGrid;
int pitch;

SDL_Window *gameWindow;
SDL_Renderer *gameRenderer;
SDL_Texture *gameTexture;

char startGame = 0;
char stepForward = 0;
int initGOL = 0;

double updateTimer = 0;

const int COLUMNS = CAM_WIDTH/8;
const int ROWS = CAM_HEIGHT/8;

const int PIXEL_WIDTH = CAM_WIDTH/COLUMNS;
const int PIXEL_HEIGHT = CAM_HEIGHT/ROWS;

const int GRID_SIZE = ROWS*COLUMNS;

int mouseX, mouseY;
char mouseClick = 0;
char clickTimer = 0;

char Rule(Pixel* grid, int index)
{
    int x_pos = index%COLUMNS;
    int y_pos = (int)index/COLUMNS;

    char alive_neig = 0;
    char isAlive = (int)grid[index].R/255;

    for (int x = x_pos-1; x <= x_pos+1; x++)
        for (int y = y_pos-1; y <= y_pos+1; y++)
            if ((x != x_pos || y != y_pos) && (x >= 0 && x <= COLUMNS && y >= 0 && y <= ROWS ))
                alive_neig += (int)grid[x + y*COLUMNS].R/255;
        
    // Cell is alive
    if (isAlive)
    {
        if (alive_neig == 3 || alive_neig == 2)
            return 1;
        else
            return 0;
    }

    // Cell is dead
    if (alive_neig == 3)
        return 1;

    return 0;
}


void GridUpdate(Pixel* tempGrid, Pixel* lastGrid, double deltaTime)
{
    // Update Grid Here
    for (int i = 0; i < GRID_SIZE; i++)
    {
        Pixel* pix = &tempGrid[i];

        char alive = Rule(lastGrid, i);

        pix->R = alive*255;
        pix->G = alive*255;
        pix->B = alive*255;
    }
}

int Update(double deltaTime)
{
    updateTimer += deltaTime;

    if (startGame == 0 && stepForward == 0)
        return 0;

    if (updateTimer < UPDATE_TIME)
        return 0;

    updateTimer = 0;

    // Locking Texture to current buffer
    if (SDL_LockTexture(gameTexture, NULL, &bufferGrid[0], &pitch) < 0)
    {
        printf("Could not lock texture: %s\n", SDL_GetError());
        return -1;
    }

    Pixel* grid = (Pixel*)bufferGrid[0];
    Pixel* lastGrid = (Pixel*)bufferGrid[1]; 

    GridUpdate(grid, lastGrid, deltaTime);

    memcpy(lastGrid, grid, GRID_SIZE*sizeof(Pixel));

    // Unlocking Texture
    SDL_UnlockTexture(gameTexture);

    return 0;
}

void Draw()
{
    // Renders texture to screen
    SDL_RenderCopy(gameRenderer, gameTexture, NULL, NULL);
    SDL_RenderPresent(gameRenderer);
}

int InitSDL()
{ 
    // Initialize window
    SDL_Init(SDL_INIT_VIDEO);

    // Creating window
    gameWindow = SDL_CreateWindow("Window",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  CAM_WIDTH,
                                  CAM_HEIGHT,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (gameWindow == NULL) 
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return -1;
    }

    // Creating renderer window
    gameRenderer = SDL_CreateRenderer(gameWindow, -1, SDL_RENDERER_ACCELERATED);

    if (gameRenderer == NULL)
    {
        printf("Could not create renderer: %s\n", SDL_GetError());
        return -1;
    }

    // Creating streaming texture
    gameTexture = SDL_CreateTexture(gameRenderer, 
                                    SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    COLUMNS,
                                    ROWS); 
    
    if (gameTexture == NULL)
    {
        printf("Could not create texture: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

void SetPixValues(int x, int y, Pixel* grid)
{
    Pixel* p = &grid[x+y*COLUMNS];
    p->R = 255;
    p->G = 255;
    p->B = 255;
}

// Return 1 on success, -1 on error, else 0
int InitGameOfLife()
{
    if (startGame == 1)
        return 1;

    if (mouseClick == 1)
    {
        if (SDL_LockTexture(gameTexture, NULL, &bufferGrid[0], &pitch) < 0)
        {
            printf("Could not lock texture: %s\n", SDL_GetError());
            return -1;
        }

        Pixel* grid = (Pixel*)bufferGrid[0];
        Pixel* lastGrid = (Pixel*)bufferGrid[1];

        memcpy(grid, lastGrid, GRID_SIZE*sizeof(Pixel));

        SetPixValues(mouseX/PIXEL_WIDTH, mouseY/PIXEL_HEIGHT, grid);

        memcpy(lastGrid, grid, GRID_SIZE*sizeof(Pixel));

        SDL_UnlockTexture(gameTexture);

        Draw();
    }

    return 0;
}

int InitGameOfLifeCode()
{
    if (SDL_LockTexture(gameTexture, NULL, &bufferGrid[0], &pitch) < 0)
    {
        printf("Could not lock texture: %s\n", SDL_GetError());
        return -1;
    }

    Pixel* grid = (Pixel*)bufferGrid[0];
    Pixel* lastGrid = (Pixel*)bufferGrid[1];

    SetPixValues(100, 101, grid);
    SetPixValues(101, 100, grid);
    SetPixValues(102, 100, grid);
    SetPixValues(102, 101, grid);
    SetPixValues(102, 102, grid);

    SetPixValues(49, 50, grid);
    SetPixValues(50, 50, grid);
    SetPixValues(50, 51, grid);
    SetPixValues(51, 50, grid);

    memcpy(lastGrid, grid, GRID_SIZE*sizeof(Pixel));

    SDL_UnlockTexture(gameTexture);

    Draw();

    return 1;
}

int Reset()
{
    free(bufferGrid);

    bufferGrid = malloc(sizeof(void*));
    for (int i = 1; i < NUM_BUFFERS; i++)
        bufferGrid[i] = (int*)calloc(GRID_SIZE, sizeof(Pixel));

    if (SDL_LockTexture(gameTexture, NULL, &bufferGrid[0], &pitch) < 0)
    {
        printf("Could not lock texture: %s\n", SDL_GetError());
        return -1;
    }
    SDL_UnlockTexture(gameTexture);

    initGOL = 0;
    startGame = 0;
    
    Draw();

    return 0;
}

int GameWindow()
{
    if (InitSDL() < 0)
        return -1;

    bufferGrid = malloc(sizeof(void*));

    for (int i = 0; i < NUM_BUFFERS; i++)
        bufferGrid[i] = (int*)calloc(GRID_SIZE, sizeof(Pixel));

    if (InitGameOfLife() < 0)
        return -1;

    Pixel* grid = (Pixel*)bufferGrid[0];

    // General Event Structure
    SDL_Event e;            
                            
    clock_t time1 = clock();
    clock_t time2 = clock();

    double deltaTime = 0;
    double fps = 0;
    char quit = 0;

    // Event loop
    while (quit == 0){
        time2 = clock();

        deltaTime = (double)(time2-time1)/CLOCKS_PER_SEC;
        fps = 1/deltaTime;

        stepForward = 0;
        mouseClick = 0;
        
        // Wait before start
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    quit = 1;
                    break;

                case SDL_KEYDOWN:
                    // Quitting if Esc is pressed
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                        quit = 1;

                    // Start/Pause
                    if (e.key.keysym.sym == SDLK_SPACE && e.key.repeat == 0)
                        startGame = !startGame;

                    if (e.key.keysym.sym == SDLK_RIGHT)
                        stepForward = 1;

                    if (e.key.keysym.sym == SDL_BUTTON_LEFT)
                    {
                        printf("hello\n");
                        mouseClick = 1;
                        SDL_GetMouseState(&mouseX, &mouseY);
                    }

                    if (e.key.keysym.sym == SDLK_r)
                        if (Reset() > 0)
                            return -1;

                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT)
                    {
                        mouseClick = 1;
                        SDL_GetMouseState(&mouseX, &mouseY);
                    }

                    break;
            }
        }

        if (initGOL != 1)
            initGOL = InitGameOfLife();
        else if (initGOL < 0)
            return -1;

        if (initGOL == 1) 
        {
            // Updating Grid
            if (Update(deltaTime) < 0)
                return -1;
            
            // Drawing Grid
            Draw();
        }

        printf("FPS:%f\r", fps);
        fflush(stdout);

        time1 = time2;
    }

    printf("\n");

    // Free allocated memory
    free(bufferGrid);

    // Destroy texture
    SDL_DestroyTexture(gameTexture);

    // Destroying rederer
    SDL_DestroyRenderer(gameRenderer);

    // Destroying window
    SDL_DestroyWindow(gameWindow);

    // Quitting SDL ...
    SDL_Quit();

    return 0;
}

int main(void)
{
    int ExitCode = GameWindow();
    return ExitCode;
}
