import unittest

from client import validate_config


class ValidationTests(unittest.TestCase):
    def test_valid_config(self):
        validate_config({"sensor_id": "s1", "value": 21.5, "unit": "C"})

    def test_wrong_value_type(self):
        with self.assertRaises(ValueError):
            validate_config({"sensor_id": "s1", "value": "warm", "unit": "C"})


if __name__ == "__main__":
    unittest.main()
