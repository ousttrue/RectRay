#pragma once

class Triangle
{
  struct TriangleImpl *m_impl;
public:
  Triangle();
  ~Triangle();
  void Render();
};

