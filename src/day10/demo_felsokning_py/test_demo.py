import unittest

from client import percentile_nearest_rank


class PercentileTests(unittest.TestCase):
    def test_nearest_rank(self):
        self.assertEqual(percentile_nearest_rank([10, 11, 12, 13, 500], 95), 500)

    def test_unsorted_input(self):
        self.assertEqual(percentile_nearest_rank([4, 1, 3, 2], 50), 2)


if __name__ == "__main__":
    unittest.main()
