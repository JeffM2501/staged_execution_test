#pragma once

#include "raylib.h"

#include <vector>
#include <string>

class ValueTracker
{
public:
    std::vector<float> Values;
    int NextValueIndex = 0;
    std::string Name;
    std::string Suffix = "ms";

    float Max = 0;
    float Min = 0;
    float ValueScale = 1000.0f;
public:

    ValueTracker(size_t maxValues, std::string_view name = "Value")
        :Name(name)
    {
        Values.resize(maxValues);
    }

    void AddValue(float value)
    {
        Values[NextValueIndex] = value;
        NextValueIndex++;
        if (NextValueIndex >= Values.size())
        {
            NextValueIndex = 0;
        }

        Max = -100;
        Min = 100;
        for (float v : Values)
        {
            if (v > Max)
                Max = v;
            if (v < Min)
                Min = v;
        }
    }

    void DrawGraph(Rectangle bounds)
    {
        DrawRectangleRec(bounds, DARKGRAY);
        DrawRectangleLinesEx(bounds, 1, GRAY);
        float heightIncrement = bounds.height / (Max - Min);
        float widthIncrement = bounds.width / Values.size();

        auto lineFunc = [&](float v1, int p1, float v2, int p2)
            {
                Vector2 lastPoint = { bounds.x + p1 * widthIncrement, bounds.y + bounds.height - (v1 - Min) * heightIncrement };
                Vector2 thisPoint = { bounds.x + p2 * widthIncrement, bounds.y + bounds.height - (v2 - Min) * heightIncrement };
                DrawLineV(lastPoint, thisPoint, GREEN);
            };

        int pointIndex = 0;
        for (int i = NextValueIndex; i < Values.size() - 1; i++)
        {
            lineFunc(Values[i], pointIndex, Values[i + 1], pointIndex + 1);
            pointIndex++;
        }

        for (int i = 0; i < NextValueIndex - 2; i++)
        {
            lineFunc(Values[i], pointIndex, Values[i + 1], pointIndex + 1);
            pointIndex++;
        }

        if (NextValueIndex > 0)
        {
            const char* text = TextFormat("%s\nCurrent %0.3f%s\nMax %0.3f%s\nMin %0.3f%s", 
                Name.c_str(), 
                Values[NextValueIndex - 1] * ValueScale, Suffix.c_str(),
                Max * ValueScale, Suffix.c_str(),
                Min * ValueScale, Suffix.c_str());

            DrawText(text, int(bounds.x + bounds.width + 2), int(bounds.y), 10, LIGHTGRAY);

        }
    }
};