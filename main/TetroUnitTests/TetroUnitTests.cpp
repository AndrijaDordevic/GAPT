#include "pch.h"
#include "CppUnitTest.h"
#include "../main/Tetromino.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TetroUnitTests
{
	TEST_CLASS(TetroUnitTests)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{// Test the Tetromino class
			spawnX = 0;
			Assert::AreEqual(spawnX, 0);
		}
	};
}
