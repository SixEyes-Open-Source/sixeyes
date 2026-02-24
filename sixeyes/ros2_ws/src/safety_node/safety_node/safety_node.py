#!/usr/bin/env python3
"""Safety node stub enforcing heartbeat policy and motor disable/enable commands."""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String

class SafetyNode(Node):
    def __init__(self):
        super().__init__('safety_node')
        self.get_logger().info('safety_node started (stub)')

def main(args=None):
    rclpy.init(args=args)
    node = SafetyNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
