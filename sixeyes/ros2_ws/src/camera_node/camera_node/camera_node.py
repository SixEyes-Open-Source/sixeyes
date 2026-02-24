#!/usr/bin/env python3
"""Camera node stub publishing /camera/image_raw using OpenCV (if available)."""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image, CameraInfo

class CameraNode(Node):
    def __init__(self):
        super().__init__('camera_node')
        self.get_logger().info('camera_node started (stub)')

def main(args=None):
    rclpy.init(args=args)
    node = CameraNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
