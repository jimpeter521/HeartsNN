#pragma once

void printStack(const char* exp, const char* file, int line);

#define demand(exp) do { if (!(exp)) { printStack(#exp, __FILE__, __LINE__); exit(1); }} while(0)
