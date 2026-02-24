#!/usr/bin/env python3
"""USB-CDC bridge stub: reads telemetry and republishes as ROS2 topics."""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import String

class UsbBridgeNode(Node):
    def __init__(self):
        super().__init__('usb_bridge_node')
        self.pub_states = self.create_publisher(JointState, '/sixeyes/joint_states', 10)
        self.pub_telemetry = self.create_publisher(String, '/sixeyes/telemetry', 10)
        self.get_logger().info('usb_bridge_node started (stub)')

    def _mock_publish(self):
        js = JointState()
        js.header.stamp = self.get_clock().now().to_msg()
        js.name = ['joint1','joint2','joint3','joint4']
        js.position = [0.0,0.0,0.0,0.0]
        self.pub_states.publish(js)

def main(args=None):
    rclpy.init(args=args)
    node = UsbBridgeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
