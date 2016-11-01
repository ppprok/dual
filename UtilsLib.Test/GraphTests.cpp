#include "stdafx.h"
#include "CppUnitTest.h"
#include "bit_matrix.h"
#include "graph_algorithms.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{
	TEST_CLASS(GraphTests)
	{
	public:
		
		TEST_METHOD(TopologicalOrderTest)
		{
			bit_matrix adj;
            adj.zero(7,7);
            adj.set(0,2);
            adj.set(2,3);
            adj.set(1,2);
            adj.set(1,3);
            adj.set(2,4);
            adj.set(5,4);
            adj.set(4,6);

            std::vector<int> order;
            topological_order(adj, order);
            Assert::AreEqual(7, (int)order.size());
            Assert::IsTrue(is_topological_order(adj, order));

            std::iter_swap(order.begin(), order.rbegin());
            Assert::IsFalse(is_topological_order(adj, order));
		}


        TEST_METHOD(TopologicalOrderNotAcyclicTest)
        {
            bit_matrix adj;
            adj.zero(7,7);
            adj.set(0,2);
            adj.set(2,3);
            adj.set(1,2);
            adj.set(1,3);
            adj.set(2,4);
            adj.set(5,4);
            adj.set(4,6);
            adj.set(6,1);

            std::vector<int> order;
            
            try
            {
                topological_order(adj, order);
                Assert::Fail();
            }
            catch(not_acyclic_graph_error&)
            {
            }
            
            
        }

        TEST_METHOD(TransitiveClosureTest)
        {
             bit_matrix adj;
             adj.zero(7,7);
             adj.set(0,2);
             adj.set(2,3);
             adj.set(1,2);
             adj.set(1,3);
             adj.set(2,4);
             adj.set(5,4);
             adj.set(4,6);
             
             Assert::IsFalse(is_transitive_closed(adj));
             
             transitive_closure(adj);
             
             Assert::IsTrue(is_transitive_closed(adj));
             
             Assert::AreEqual(7 + 7, edges_number(adj));
             
        }


	};
}