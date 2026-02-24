#!/usr/bin/env python3
"""Joint state aggregator stub."""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState

class JointStateNode(Node):
    def __init__(self):
        super().__init__('joint_state_node')
        self.pub = self.create_publisher(JointState, '/joint_states', 10)
        self.get_logger().info('joint_state_node started (stub)')

def main(args=None):
    rclpy.init(args=args)
    node = JointStateNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
