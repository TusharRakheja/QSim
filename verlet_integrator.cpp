// verlet_integrator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <SDL.h>

class vec2d
{
public:
	float x, y;
	vec2d()
	{
		x = 0, y = 0;
	}
	vec2d(float x, float y)
	{
		this->x = x;
		this->y = y;
	}
	float operator[](int a) //SUBSCRIPT access
	{
		if (a == 0)return this->x;
		else if (a == 1)return this->y;
		else return -1;
	}
	float operator*(vec2d &b) //DOT multiplication
	{
		return this->x*b[0] + this->y * b[1];
	}
	vec2d operator*(float c) //SCALAR multiplication
	{
		vec2d r;
		r.x = c*this->x;
		r.y = c*this->y;
		return r;
	}
	vec2d operator*(int c) //SCALAR multiplication
	{
		vec2d r;
		r.x = c*this->x;
		r.y = c*this->y;
		return r;
	}
	void operator=(vec2d &b)
	{
		this->x = b.x;
		this->y = b.y;
	}
	vec2d operator+(vec2d &b)
	{
		vec2d r;
		r.x = this->x + b.x;
		r.y = this->y + b.y;
		return r;
	}

	vec2d operator-(vec2d &b)
	{
		vec2d r;
		r.x = this->x - b.x;
		r.y = this->y - b.y;
		return r;
	}
	float norm()
	{
		return SDL_pow(SDL_pow(this->x, 2) + SDL_pow(this->y, 2), 1 / 2);
	}
	float distance(vec2d &b)
	{
		float r = ((this->x - b.x)*(this->x - b.x)) + ((this->y - b.y)*(this->y - b.y) );
		return SDL_pow(r, 0.5);
	}
	float distance2(vec2d &b)
	{
		float r = ((this->x - b.x)*(this->x - b.x)) + ((this->y - b.y)*(this->y - b.y));
		return r;
	}
};


typedef struct
{
	int id;		//Unique ID
	float q;	//Charge, signed
	int fixed;	// is the charge fixed in place? = 0 no.

	vec2d r;	//Position vector at current time
	vec2d r_;	//Position vector at t-1 for verlet int
	
	vec2d r_t;  //Stores new calculated position temporarily

	vec2d v;	//Velocity vector
	vec2d v_;	//Updated velocity
	
	vec2d f;	//Force vector

	
} vParticle;

double time, delta; // current time and dt (step size).
int npart;	// number of particles

float nx_q; 
int nx_s; //Charge and sign of next particle to be added.
int fixed; //whether or not the charge is fixed in place.


vParticle List[128]; // 128 particles max. TODO : Replace with vector<Particle>

// Physics functions
void vector_euler();
void rk4_integrate();
void verlet_integrate();
void rk4_adaptive_integrate();

int _tmain(int argc, _TCHAR* argv[])
{

	npart = 0;
	nx_q = 1.0;
	nx_s = 1;
		
	puts("Init SDL...\n");
	
	SDL_Window *w = nullptr;
	SDL_Renderer *r = nullptr;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	w = SDL_CreateWindow("q-sim.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
	r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);

	bool quit = false;
	time = 0;	// Set the universe's time to zero and begin simulation
	delta = 1.0 / 60.0; // Each frame advances the time by delta amount
	
	printf("NumDrv=%d\n", SDL_GetNumRenderDrivers());

	SDL_RendererInfo inf;

	SDL_GetRendererInfo(r, &inf);

	printf("Current Renderer = %s\n", inf.name);
	printf("Current Video Driver = %s\n", SDL_GetCurrentVideoDriver());



	while (!quit)
	{
		//GUI Handling
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT) quit = true;
			
			if (e.type == SDL_MOUSEWHEEL) //mouse wheel controls charge magnitude. 
			{
				nx_q += e.wheel.y;
				if (nx_q < 1) nx_q = 1;
				printf("Next charge = %f\n", nx_q);
			}
			if (e.type == SDL_MOUSEBUTTONUP)
			{
				if (e.button.button == SDL_BUTTON_LEFT) // Left click, add a new +ve particle to the sim.
				{
					printf("Add charge +%fq at %d %d\n", nx_q, e.button.x, e.button.y);
					List[npart].id = npart;
					List[npart].r.x = e.button.x;
					List[npart].r.y = e.button.y;
					List[npart].r_.x = e.button.x;
					List[npart].r_.y = e.button.y;
					List[npart].q = nx_q;
					List[npart].fixed = fixed;
					//TODO
					npart++;
				}
				else if (e.button.button == SDL_BUTTON_RIGHT) // Right click, add -ve particle to the sim.
				{
					printf("Add charge -%fq at %d %d\n", nx_q, e.button.x, e.button.y);
					List[npart].id = npart;
					List[npart].r.x = e.button.x;
					List[npart].r.y = e.button.y;
					List[npart].r_.x = e.button.x;
					List[npart].r_.y = e.button.y;
					List[npart].q = -nx_q;
					List[npart].fixed = fixed;
					//TODO
					npart++;
				}
				else if (e.button.button == SDL_BUTTON_MIDDLE) // Middle-click toggles fixed masses
				{
					fixed = 1 - fixed;
					printf("Fixed status is now %d\n",fixed);
					
				}
			}
		}

		//Physics calculations

		//vector_euler();
		verlet_integrate(); //Superior


		//Render to screen

		SDL_SetRenderDrawColor(r, 255, 255, 255, SDL_ALPHA_OPAQUE); // Clear screen to white color.
		SDL_RenderClear(r);

		//Loop over and draw the points

		SDL_Rect pos;
		vParticle *p = nullptr;

		for (int i = 0; i < npart; i++)
		{
			p = &List[i];
			pos = { p->r[0] - 1 * SDL_abs(p->q), p->r[1] - 1 * SDL_abs(p->q), 2*SDL_abs(p->q), 2*SDL_abs(p->q) };
			p->q > 0 ? SDL_SetRenderDrawColor(r,255, 0, 0, SDL_ALPHA_OPAQUE) : SDL_SetRenderDrawColor(r,0, 0, 255, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(r, &pos);
		} //^^Depending on the charge's magnitude and polarity, render a box centered at the x,y coordinates
		
		SDL_RenderPresent(r); //Render to the screen

	}

	SDL_DestroyRenderer(r); // Clean up allocated SDL resources
	SDL_DestroyWindow(w);
	SDL_Quit();


	return 0;
}
/*
void euler_integrate()
{
	//Find net force on each particle
	// E-field at (x,y) E_x,y = SIGMA q/r^2
	// f_q = q * E
	// a = f/m ; m = 1 and gravity is not considered
	// v = v + a*dt
	// r = r + v*dt

	Particle *p = nullptr;

	for (int i = 0; i < npart; i++)
	{
		p = &World[i];

		float Ex = 0;
		float Ey = 0;

		for (int j = 0; j < npart; j++)
		{
			if (World[j].id == p->id) continue;

			float r2 = SDL_pow(p->vec_r[0] - World[j].vec_r[0], 2) + SDL_pow(p->vec_r[1] - World[j].vec_r[1], 2);
			Ex += (World[j].q) * (p->vec_r[0]) / SDL_pow(r2, 1.5);
			Ey += (World[j].q) * (p->vec_r[1]) / SDL_pow(r2, 1.5);
		}
		
		p->vec_f[0] = (p->q)*Ex;
		p->vec_f[1] = (p->q)*Ey;

		p->vec_v[0] = p->vec_v[0] + delta * p->vec_f[0]; // a = f
		p->vec_v[1] = p->vec_v[1] + delta * p->vec_f[1];

		p->vec_rn[0] = p->vec_r[0] + delta * p->vec_v[0];
		p->vec_rn[1] = p->vec_r[1] + delta * p->vec_v[1];
	
	} //finished calculating new positions, now update.

	for (int i = 0; i < npart; i++)
	{
		p = &World[i];
		p->vec_r[0] = p->vec_rn[0];
		p->vec_r[1] = p->vec_rn[1];
	}
	//oh god i'm ashamed of this
}
*/
void vector_euler() ///EXPLICIT EULER INTEGRATIONS
{
	vParticle *vp = nullptr;

	for (int i = 0; i < npart; i++)
	{
		vp = &List[i];

		vec2d netE(0,0);

		for (int j = 0; j < npart; j++)
		{
			if (vp->id == List[j].id) continue;
			netE = netE + (vp->r - List[j].r) * (List[j].q / vp->r.distance2(List[j].r));
		}

		vp->f = netE * vp->q;
		vp->v = vp->v + vp->f*(float)delta;
		vp->r_ = vp->r + vp->v*(float)delta;
		
	}
	//update positions
	for (int i = 0; i < npart; i++)
	{
		vp = &List[i];
		vp->r = vp->r_;
	}


}

void verlet_integrate()
{
	vParticle *vp = nullptr;

	for (int i = 0; i < npart; i++) //Foreach particle in the simulation..
	{
		vp = &List[i];

		if (vp->fixed == 1)continue;

		vec2d netE(0, 0);

		for (int j = 0; j < npart; j++) //Calculate the net E field at the particle coords
		{
			if (vp->id == List[j].id) continue; //Do not include ourselves in the calculation for net E-field
			
			netE = netE + (vp->r - List[j].r) * (List[j].q / vp->r.distance2(List[j].r));
		}
		vp->f = netE * vp->q; //The net force on the particle, F=qE
		//Here, the accn = force as mass = 1 and gravitational effects are not considered.
		//actually on further consideration, let mass = abs(q) makes for more interesting sims.

		vp->r_t = (vp->r) * 2 - vp->r_ + vp->f*((float)delta*(float)delta / SDL_abs(vp->q));
	}
	//Updates
	for (int i = 0; i < npart; i++)
	{
		vp = &List[i];
		if (vp->fixed == 1)continue;
		vp->r_ = vp->r;
		vp->r = vp->r_t;
	}


}