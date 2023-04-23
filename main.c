#include<stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define WIDTH 20
#define HEIGHT 20
#define NUM_FLAKES 40

struct Cell {
    double posX;
    double posY;
    int cellWidth;
    int cellHeight;
    int depth;
    struct Flakes*  flakes; // Hält die zu der Zelle zugehörigen Flakes
    struct Cell* cells;
    /*
        // Lösche den Teilbaum ab und inklusive dieser Zelle
        clearAll() {
            this.cells = [];
            this.flakes = [];
        }

        // Gib aus ob die Zelle Kinder hat
        hasChildren() {
            return this.cells.length > 0;
        }

        // Erzeuge Kinder-Zellen
        createChildren() {
            let cx = this.posX;
            let cy = this.posY;
            let cw = this.cellWidth/2;
            let ch = this.cellHeight/2;

            this.cells[0] = new Cell(cx, cy, cw, ch, this.depth + 1);
            this.cells[1] = new Cell(cx, cy + ch, cw, ch, this.depth + 1);
            this.cells[2] = new Cell(cx + cw, cy, cw, ch, this.depth + 1);
            this.cells[3] = new Cell(cx + cw, cy + ch, cw, ch, this.depth + 1);
        }

        // Füge die Flake(-id) den passenden Kinder-Zellen hinzu
        addToChildren(flake_id) {
            // Erhalte die Position des Flake-Objekts mit ID: "flake_id"
            // aus dem globalen bowl-Array
            let fx = bowl[flake_id].x;
            let fy = bowl[flake_id].y;

            // Iteriere über die Anzahl der Kinder-Zellen
            // Sollte Flake in mehreren Zellen liegen,
            // füge sie allen Betroffenen hinzu,
            // damit Collision auch Zellen-Übergreifend möglich ist.
            for (let i = 0; i < 4; i++) {
                let cx = this.cells[i].posX;
                let cy = this.cells[i].posY;
                let cw = this.cells[i].cellWidth;
                let ch = this.cells[i].cellHeight;

                // Falls Flake "flake_id" Positionsmäßig in Zelle i liegt, adde sie dort
                if (fx + flakeSize > cx && fx < cx + cw && fy + flakeSize > cy && fy < cy + ch)
                    this.cells[i].addFlake(flake_id);
            }
        }

        // Füge dem Baum eine Flake(-id) hinzu
        addFlake(flake_id) {
            // Falls Zelle Kinder-Zellen hat, übergebe die Flake an die,
            // sonst adde sie zu den Flakes dieser Zelle
            if (this.hasChildren()) {
                this.addToChildren(flake_id);
                return;
            }

            // Falls Flakesanzahl in Zelle die max Anzahl einer Zelle
            // überschreitet oder die max Tiefe erreicht wurde,
            // erzeuge Kinder-Zellen
            // sonst Prüfe auf Kollisionen
            // (Kein extra Aufruf nötig: Zwei Fliegen mit einer Klappe)
            if (this.flakes.length + 1 <= maxFlakesPerCell || this.depth >= maxDepth) {
                // Prüfe auf Kollisionen mit dieser Flake in dieser Zelle
                this.checkCollision(flake_id);

                // Füge die Flake endgültig der Zelle hinzu
                this.flakes[this.flakes.length] = flake_id;
            } else {
                // Erzeuge neue Kinder-Zellen
                this.createChildren();

                // Übergebe alle Flakes der Zelle den Kinder-Zellen
                for (let i = 0; i < this.flakes.length; i++)
                    this.addToChildren(this.flakes[i]);

                this.addToChildren(flake_id);
            }
        }

        //Prüfe auf Kollisionen einer Flake mit allen aus der Zelle
        checkCollision(flake_id) {
            let midX1 = bowl[flake_id].x;
            let midY1 = bowl[flake_id].y;

            for (let i = 0; i < this.flakes.length; i++) {
                let midX2 = bowl[this.flakes[i]].x;
                let midY2 = bowl[this.flakes[i]].y;

                let a = midX2 - midX1;
                let b = midY2 - midY1;
                let c = sqrt(a*a + b*b);

                if (c > flakeSize)
                    continue;

                let angle = atan2(b, a);
                let targetX = midX1 + cos(angle) * flakeSize;
                let targetY = midY1 + sin(angle) * flakeSize;
                let ax = (targetX - midX2);
                let ay = (targetY - midY2);

                bowl[flake_id].velX -= ax * force;
                bowl[flake_id].velY -= ay * force;
                bowl[this.flakes[i]].velX += ax * force;
                bowl[this.flakes[i]].velY += ay * force;
            }
        }

        // Male den Teilbaum
        drawCells() {
            if (this.hasChildren())
                for (let i = 0; i < 4; i++)
                    this.cells[i].drawCells();
            else {
                rect(this.posX, this.posY, this.cellWidth, this.cellHeight);

                if (showFlakesCounter) {
                    text(this.flakes.length, this.posX + this.cellWidth / 2 - 3, this.posY + this.cellHeight / 2 + 5);
                }
            }
        }*/
};

// Klasse für Kugel (=Flake)
struct Flake {
    int x;
    int y;
    int velX;
    int velY;
    /*
    move() { //Mache einen Schritt
        if (this.velX > maxSpeed)
            this.velX = maxSpeed;
        else if (this.velX < -maxSpeed)
            this.velX = -maxSpeed;

        if (this.velY > maxSpeed)
            this.velY = maxSpeed;
        else if (this.velY < -maxSpeed)
            this.velY = -maxSpeed;

        this.velY += gravity;

        if (this.x + this.velX + flakeSize/2 > width) {
            this.x = width - flakeSize/2;
            this.velX *= -1;
        } else if (this.x + this.velX - flakeSize/2 < 0) {
            this.x = flakeSize/2;
            this.velX *= -1;
        }

        if (this.y + this.velY + flakeSize/2 > height) {
            this.y = height - flakeSize/2;
            this.velY *= -1;
        } else if (this.y + this.velY - flakeSize/2 < 0) {
            this.y = flakeSize/2;
            this.velY *= -1;
        }

        this.x += this.velX;
        this.y += this.velY;
    }
}*/

};
struct Cell* Cell_constructor(int posX, int posY, int cellWidth, int cellHeight, int depth);
struct Flake* Flake_constructor(int x, int y, int maxSpeed);
void save_Iteration(FILE* file, struct Cell array[]);

int main() {

    FILE* file = fopen("data.txt","w");

    if(file == NULL)
    {
        printf("Error!");
        exit(1);
    }

    srand(time(NULL));
    struct Cell bowl[10000]; //Flakearray
// Bowl (oder =Müslischale) enthält alle Kugeln (oder =Cornflakes)
    int maxSpeed = 1; //Max Geschwindigkeit der Flake
    double force = 1.0; //Kraftfaktor des Wegstoßens einer Flake bei Kollision
    float gravity = 0; //Gravitation

    bool showFPS = true;
    bool showCells = true; // Zeichne Zellen des Baumes (High Performance Loss!!)
    bool showFlakesCounter = false; // Zeichne Text: Anzahl der Flakes einer Zelle (High Performance Loss!!)

    double flakeSize = 0; // Größe der Flakes (für auto-ermittlung: = 0 setzen
    int maxDepth = 7; //Maximale Tiefe des Baumes (für auto-ermittlung: = 0 setzen)
    int maxFlakesPerCell = 6; //Maximale Anzahl der Flakes, die eine zelle halten kann -> ab dann neue kinderzellen ("10" wäre zu lang für eine kleine Zelle zum Anzeigen, daher max "9")

    int timer = 1000;
    int iterationCounter = 0;
    int lastIterationCounter = 0;

    int updateDelay = 16; // Update Intervall
    int updateTimer = 0; // Zeit seit Programmstart bis zum nächsten Update

    //createCanvas(1000, 1000);

    // Passende Flake-Größe abhängig von Anzahl der Flakes
    if (flakeSize == 0)
        flakeSize = 1000 / (sqrt(NUM_FLAKES) * 10);

    // Maximale Tiefe des Baumes. Effizienz-Tests versprechen bei log(6) die beste Performance
    if (maxDepth == 0)
        maxDepth = (int) (log(NUM_FLAKES) / log(6));


    // Testing
    //console.log("Circles: " + num_flakes)
    //console.log("Circle Diameter: " + flakeSize)
    //console.log("Max Circles p. Cell: " + maxFlakesPerCell);

    //Iteriere "Anzahl der Kugeln"-mal und füge der Müslischüssel immer eine neue Cornflake mit zufälliger Position hinzu
    for (int i = 0; i < NUM_FLAKES; i++) {
        //bowl[i] = Flake(random(0, width), random(0, height));
        bowl[i].posX = (int) (rand() / (float)RAND_MAX * WIDTH);
        bowl[i].posY = (int) (rand() / (float)RAND_MAX * HEIGHT);
    }


    //Erzeuge Wurzel-Zelle des Baumes (gesamte Zeichenfläche)
    struct Cell* rootCell = Cell_constructor(0,0 , WIDTH,HEIGHT,0);

    // weißen Malstift auswählen, Shapes nicht füllen und Malstift-Thickness setzen
    //strokeWeight(flakeSize);
    //noFill();

    save_Iteration(file, bowl);


    for(int i=0; i<1000; i++ ) {
        for(int j=0; j<NUM_FLAKES; j++ ) {
            bowl[j].posX += (((rand()/(float)RAND_MAX)-0.5))/20;
            bowl[j].posY += (((rand()/(float)RAND_MAX)-0.5))/20;
        }
        save_Iteration(file, bowl);
    }


    fclose(file);
}


// Klasse für eine Zelle des Quadtrees



struct Cell* Cell_constructor(int posX, int posY, int cellWidth, int cellHeight, int depth) {
    struct Cell* this = malloc(sizeof(struct Cell));
    this->posX = posX;
    this->posY = posY;
    this->cellWidth = cellWidth;
    this->cellHeight = cellHeight;
    this->depth = depth;
    //this->flakes = []; // Hält die zu der Zelle zugehörigen Flakes
    //this->cells = []; // Hält Kinder-Zellen (leer oder Länge 4)
    return this;
}




struct Flake* Flake_constructor(int x, int y, int maxSpeed) { //Erzeuge Flake
    struct Flake* this = malloc(sizeof(struct Flake));
    this->x = x;
    this->y = y;
    this->velX = 0;//random(-maxSpeed, maxSpeed);
    this->velY = 0;//random(-maxSpeed, maxSpeed);

    return this;
}

void save_Iteration(FILE* file, struct Cell array[] ) {
    for(int i=0; i<NUM_FLAKES; i++) {
        fprintf(file,"%f %f ",array[i].posX, array[i].posY);
    }
    fprintf(file,"%s", " \n");
}