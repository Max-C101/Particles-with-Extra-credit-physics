#include "Particle.h"
#include "Matrices.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;
using namespace sf;
using namespace Matrices;

Particle::Particle(RenderTarget& target, int numPoints, Vector2i mouseClickPosition) : m_A(2, numPoints)
{
    m_ttl = 10.0;//temp
    m_numPoints = numPoints;

    m_radiansPerSec = ((float)rand() / (RAND_MAX)) * M_PI;
    if (rand() % 2 != 0) { m_radiansPerSec *= -1.0f; }//If true (non-zero), multiply by -1

    m_cartesianPlane.setCenter(0, 0);
    m_cartesianPlane.setSize(target.getSize().x, (1.0) * target.getSize().y);
    m_centerCoordinate = target.mapPixelToCoords(mouseClickPosition, m_cartesianPlane);

    m_vx = (float)(rand() % 101 + 20); // Generate positive magnitude 
    if (rand() % 2 != 0)
    {
        m_vx *= -1.0f; // If true (non-zero), multiply by -1	      // can experiment wit both 
    }

    m_vy = (float)(rand() % 101 + 20);
    if (rand() % 2 != 0)
    {
        m_vy *= -1.0f; // If true (non-zero), particle moves DOWN initially
    }
    m_color1 = Color(255, 255, 255);
    m_color2 = Color(rand() % 256, rand() % 256, rand() % 256);  // changed this from: m_color2 = Color(rand(), rand(), rand());

    double theta = ((double)rand() / (RAND_MAX)) * (M_PI / 2.0);
    double dtheta = 2.0 * M_PI / (m_numPoints - 1);
    for (int j = 0; j < numPoints; j++)
    {
        double r, dx, dy;
        r = (rand() % (80 - 20 + 1) + 20);
        dx = r * cos(theta);
        dy = r * sin(theta);
        m_A(0, j) = m_centerCoordinate.x + dx;
        m_A(1, j) = m_centerCoordinate.y + dy;
        theta += dtheta;
    }
}
// ----- transformation Impletmentations -----   //

void Particle::translate(double xShift, double yShift)
{
    TranslationMatrix T(xShift, yShift, m_numPoints);
    m_A = T + m_A; // m_A = T + m_A
    m_centerCoordinate.x += (float)xShift;
    m_centerCoordinate.y += (float)yShift;
}

void Particle::rotate(double theta)
{
    // 1. Shift to Origin
    sf::Vector2f temp = m_centerCoordinate;

    TranslationMatrix T_origin(-m_centerCoordinate.x, -m_centerCoordinate.y, m_A.getCols());
    m_A = T_origin + m_A; // m_A = T_origin + m_A

    // 2. Rotate
    RotationMatrix R(theta);
    m_A = R * m_A; // m_A = R * m_A (Left-multiplication)

    // 3. Shift Back
    TranslationMatrix T_back(temp.x, temp.y, m_A.getCols());
    m_A = T_back + m_A;
}
void Particle::scale(double c)
{
    // 1. Shift to Origin
    sf::Vector2f temp = m_centerCoordinate;

    TranslationMatrix T_origin(-m_centerCoordinate.x, -m_centerCoordinate.y, m_A.getCols());
    m_A = T_origin + m_A;

    // 2. Scale
    ScalingMatrix S(c);
    m_A = S * m_A; // m_A = S * m_A (Left-multiplication)

    // 3. Shift Back
    TranslationMatrix T_back(temp.x, temp.y, m_A.getCols());
    m_A = T_back + m_A;
}

//-------Draw and update functions----//

void Particle::draw(RenderTarget& target, RenderStates states) const
{
    // Use TriangleFan, size is m_numPoints + 1 (center of vertex)
    sf::VertexArray lines(sf::TriangleFan, m_numPoints + 1);


    sf::Vector2f center = (Vector2f)target.mapCoordsToPixel(m_centerCoordinate, m_cartesianPlane);

    // Element 0: Particle Center
    lines[0].position = center;
    lines[0].color = m_color1;

    // Elements 1 to m_numPoints: Vertices (Outer perimeter)
    for (int j = 1; j <= m_numPoints; ++j) {
        // Get Cartesian coordinates from column j - 1 in m_A
        sf::Vector2f cartesianCoord = { (float)m_A(0, j - 1), (float)m_A(1, j - 1) };

        // Map Cartesian coordinate to pixel coordinates
        lines[j].position = (Vector2f)target.mapCoordsToPixel(cartesianCoord, m_cartesianPlane);
        lines[j].color = m_color2;
    }
    target.draw(lines, states);
}

void Particle::update(float dt, const sf::Vector2f& magnetPos, float forceSign)
{
    // 1. Decrease TTL (time to live)
    //m_ttl -= dt;
    // 2. Rotation
    rotate(dt * m_radiansPerSec);

    // --- Magnetic force calculation ---//

    if (std::abs(forceSign) > 0.01f) // appied force if button is held
    {
        float dx_displacement = magnetPos.x - m_centerCoordinate.x;
        float dy_displacement = magnetPos.y - m_centerCoordinate.y;
        float distance = std::hypot(dx_displacement, dy_displacement); //??

        if (distance > 1.0f) // div by 0 avoid
        {
            //calculate the unit vector
            float unit_x = dx_displacement / distance;
            float unit_y = dy_displacement / distance;

            // distance falloff 

            //Appling force; F_MAG is magnitude, unit vecot is a direction, forceSign is +/-
            m_vx += F_MAG * unit_x * forceSign * dt;
            m_vy += F_MAG * unit_y * forceSign * dt;

        }
    }
    // Translation (Movement and Gravity)
    m_vy += G * dt; //apply Gravity (G) to vertical velocity

    float dx = m_vx * dt; // Horizontal distance
    float dy = m_vy * dt; // Vertical distance

    translate(dx, dy);

    const float LS_X_COORD = -m_cartesianPlane.getSize().x / 2.0f;//temp
    const float RS_X_COORD = m_cartesianPlane.getSize().x / 2.0f;
    const float ROOF_Y_COORD = -m_cartesianPlane.getSize().y / 2.0f; //should maybe be top edge in Cartesian y
    const float FLOOR_Y_COORD = m_cartesianPlane.getSize().y / 2.0f; // This is the bottom edge in Cartesian Y
    //const float ELASTICITY = 0.8f; // Lose 20% of energy on bounce
    //temp
    const float ELASTICITY = 0.2f; // Lose 80% of energy on bounce

    if (m_centerCoordinate.y > FLOOR_Y_COORD)
    {
        m_vy = -m_vy * ELASTICITY;      // Reverse velocity and lose energy
        m_centerCoordinate.y = FLOOR_Y_COORD; // Snap back to floor so it doesn't get stuck
    }
    // If Y is LESS than the roof coordinate, it hit the top
    else if (m_centerCoordinate.y < ROOF_Y_COORD)
    {
        m_vy = -m_vy * ELASTICITY;
        m_centerCoordinate.y = ROOF_Y_COORD; // Snap back to roof
    }
    // X-Axis Collision (LEFT and RIGHT Walls)
    if (m_centerCoordinate.x < LS_X_COORD)
    {
        m_vx = -m_vx * ELASTICITY;
        m_centerCoordinate.x = LS_X_COORD;
    }
    else if (m_centerCoordinate.x > RS_X_COORD)
    {
        m_vx = -m_vx * ELASTICITY;
        m_centerCoordinate.x = RS_X_COORD;
    }
}




//float Particle::getTTL() { return m_ttl; }// already defined inside of particle.h (inline)
//Functions for unit testing
bool Particle::almostEqual(double a, double b, double eps)
{
    return std::fabs(a - b) < eps;
}

//Professor's code 
void Particle::unitTests()
{
    int score = 0;
    cout << "Testing RotationMatrix constructor...";
    double theta = M_PI / 4.0;
    RotationMatrix r(M_PI / 4);
    if (r.getRows() == 2 && r.getCols() == 2 && almostEqual(r(0, 0), cos(theta))
        && almostEqual(r(0, 1), -sin(theta))
        && almostEqual(r(1, 0), sin(theta))
        && almostEqual(r(1, 1), cos(theta)))
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Testing ScalingMatrix constructor...";
    ScalingMatrix s(1.5);
    if (s.getRows() == 2 && s.getCols() == 2
        && almostEqual(s(0, 0), 1.5)
        && almostEqual(s(0, 1), 0)
        && almostEqual(s(1, 0), 0)
        && almostEqual(s(1, 1), 1.5))
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Testing TranslationMatrix constructor...";
    TranslationMatrix t(5, -5, 3);
    if (t.getRows() == 2 && t.getCols() == 3
        && almostEqual(t(0, 0), 5)
        && almostEqual(t(1, 0), -5)
        && almostEqual(t(0, 1), 5)
        && almostEqual(t(1, 1), -5)
        && almostEqual(t(0, 2), 5)
        && almostEqual(t(1, 2), -5))
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Testing Particles..." << endl;
    cout << "Testing Particle mapping to Cartesian origin..." << endl;
    if (m_centerCoordinate.x != 0 || m_centerCoordinate.y != 0)
    {
        cout << "Failed. Expected (0,0). Received: (" << m_centerCoordinate.x <<
            "," << m_centerCoordinate.y << ")" << endl;
    }
    else
    {
        cout << "Passed. +1" << endl;
        score++;
    }

    cout << "Applying one rotation of 90 degrees about the origin..." << endl;
    Matrix initialCoords = m_A;
    rotate(M_PI / 2.0);
    bool rotationPassed = true;
    for (int j = 0; j < initialCoords.getCols(); j++)
    {
        if (!almostEqual(m_A(0, j), -initialCoords(1, j)) || !almostEqual(m_A(1, j), initialCoords(0, j)))
        {
            cout << "Failed mapping: ";
            cout << "(" << initialCoords(0, j) << ", " << initialCoords(1, j) << ") ==> (" << m_A(0, j) << ", " << m_A(1, j) << ")" << endl;
            rotationPassed = false;
        }
    }
    if (rotationPassed)
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Applying a scale of 0.5..." << endl;
    initialCoords = m_A;
    scale(0.5);
    bool scalePassed = true;
    for (int j = 0; j < initialCoords.getCols(); j++)
    {
        if (!almostEqual(m_A(0, j), 0.5 * initialCoords(0, j)) || !almostEqual(m_A(1, j), 0.5 * initialCoords(1, j)))
        {
            cout << "Failed mapping: ";
            cout << "(" << initialCoords(0, j) << ", " << initialCoords(1, j) << ") ==> (" << m_A(0, j) << ", " << m_A(1, j) << ")" << endl;
            scalePassed = false;
        }
    }
    if (scalePassed)
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Applying a translation of (10, 5)..." << endl;
    initialCoords = m_A;
    translate(10, 5);
    bool translatePassed = true;
    for (int j = 0; j < initialCoords.getCols(); j++)
    {
        if (!almostEqual(m_A(0, j), 10 + initialCoords(0, j)) || !almostEqual(m_A(1, j), 5 + initialCoords(1, j)))
        {
            cout << "Failed mapping: ";
            cout << "(" << initialCoords(0, j) << ", " << initialCoords(1, j) << ") ==> (" << m_A(0, j) << ", " << m_A(1, j) << ")" << endl;
            translatePassed = false;
        }
    }
    if (translatePassed)
    {
        cout << "Passed. +1" << endl;
        score++;
    }
    else
    {
        cout << "Failed." << endl;
    }

    cout << "Score: " << score << " / 7" << endl;
}