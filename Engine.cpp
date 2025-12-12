#include "Engine.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

Engine::Engine()
{
	int screenWidth = VideoMode::getDesktopMode().width / 2; // we div by 2 just to half the window size thats it
	int screenHeight = VideoMode::getDesktopMode().height / 2;
	VideoMode vm(screenWidth, screenHeight);
	m_Window.create(vm, "Particles");
	m_Window.setFramerateLimit(60);
	font.loadFromFile("fonts/CoolveticaRg.otf");
	instruct.setFont(font);
	instruct.setCharacterSize(35);
	instruct.setFillColor(Color::Black);
	instruct.setOutlineColor(Color::White);
	instruct.setOutlineThickness(2.0f);
	instruct.setString("Left-click hold to attract, Right-click hold to repel");
}
// Run will call all the private functions
void Engine::run()
{
	Clock clock;
	cout << "Starting Particle unit tests..." << endl;
	Particle p(m_Window, 4, { (int)m_Window.getSize().x / 2, (int)m_Window.getSize().y / 2 });
	p.unitTests();
	cout << "Unit tests complete.  Starting engine..." << endl;

	Vector2i cent = { 0,0 };
	for (int i = 0; i < 50; i++)//particles 
	{
		int numPoints = rand() % (50 - 25 + 1) + 25;  //rand() % (b - a + 1) + a // basically[a...b]here its 25...50
		m_particles.emplace_back(m_Window, numPoints, Vector2i(m_Window.getSize().x / 2, m_Window.getSize().y / 2));
	}
	while (m_Window.isOpen())
	{
		Time dt = clock.restart();

		float dtAsSeconds = dt.asSeconds();

		input();
		update(dtAsSeconds);
		draw();
	}
}

void Engine::input() {
	Event event;
	while (m_Window.pollEvent(event)) {
		if (event.type == Event::Closed) m_Window.close();
		if (Keyboard::isKeyPressed(Keyboard::Escape)) m_Window.close();
	}

	// Move real-time state checks OUTSIDE the pollEvent loop
	Vector2i mousePixelPos = Mouse::getPosition(m_Window);
	m_magnetCenter = m_Window.mapPixelToCoords(mousePixelPos);

	if (Mouse::isButtonPressed(Mouse::Left)) m_forceSign = 1.0f;
	else if (Mouse::isButtonPressed(Mouse::Right)) m_forceSign = -1.0f;
	else m_forceSign = 0.0f;
}
void Engine::update(float dtAsSeconds)
{
	auto it = m_particles.begin();
	while (it != m_particles.end())
	{
		it->update(dtAsSeconds, m_magnetCenter, m_forceSign);
		it++;
	}
}


void Engine::draw()//wip(?)
{
	m_Window.clear();

	for (const auto& particle : m_particles)
	{
		m_Window.draw(particle);
	}
	m_Window.draw(instruct);
	m_Window.display();
}