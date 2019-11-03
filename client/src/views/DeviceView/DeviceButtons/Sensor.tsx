import React from 'react'
import styled from 'styled-components'
import { useDebouncedCallback } from 'use-debounce'
import {
  animated,
  useSpring,
  config as springConfig,
  interpolate
} from 'react-spring'
import { useDrag } from 'react-use-gesture'
import { clamp } from 'lodash-es'

import { colors } from '../../../utils/colors'
import { SensorType } from '../../../domain/Button'
import toPercentage from '../../../utils/toPercentage'
import scale from '../../../utils/scale'
import { DeviceDescription } from '../../../../../common-types/device'
import { DeviceInputEvent } from '../../../../../common-types/messages'
import { useServerContext } from '../../../context/SocketContext'

const Container = styled.div`
  height: 100%;
  position: relative;
`

const ThresholdBar = styled(animated.div)`
  background-color: ${colors.thresholdBar};
  bottom: 0;
  left: 0;
  position: absolute;
  right: 0;
  top: 0;
  transform-origin: 50% 100%;
  will-change: transform;
`

const OverThresholdBar = styled(animated.div)`
  background-color: ${colors.overThresholdBar};
  left: 0;
  position: absolute;
  right: 0;
  bottom: 0;
  top: 0;
  transform-origin: 50% 100%;
  will-change: transform;
`

const Bar = styled(animated.div)`
  background-color: ${colors.sensorBarColor};
  bottom: 0;
  left: 0;
  position: absolute;
  right: 0;
  top: 0;
  transform-origin: 50% 100%;
  will-change: transform;
`

const Thumb = styled(animated.div)`
  position: absolute;
  border-radius: 9999px;
  color: white;
  text-align: center;
  line-height: 1;
  transform: translate(-50%, 50%);
  background-color: white;
  left: 50%;
  color: black;
  user-select: none;
  padding: ${scale(1)} ${scale(1)};
`

interface Props {
  serverAddress: string
  device: DeviceDescription
  sensor: SensorType
  enableThresholdChange: boolean
}

const Sensor = React.memo<Props>(
  ({ serverAddress, device, sensor, enableThresholdChange }) => {
    const serverContext = useServerContext()
    const containerRef = React.useRef<HTMLDivElement>(null)
    const currentlyDownRef = React.useRef<boolean>(false)

    const [{ value: sensorValue }, setSensorValue] = useSpring(() => ({
      value: 0,
      config: { ...springConfig.stiff, clamp: true }
    }))

    const handleInputEvent = React.useCallback(
      (inputEvent: DeviceInputEvent) => {
        const value = inputEvent.inputData.sensors[sensor.sensorIndex]

        setSensorValue({
          value
        })
      },
      [sensor.sensorIndex, setSensorValue]
    )

    React.useEffect(
      () =>
        serverContext.subscribeToInputEvents(
          serverAddress,
          device.id,
          handleInputEvent
        ),
      [serverAddress, device, serverContext, handleInputEvent]
    )

    const [{ value: thresholdValue }, setThresholdValue] = useSpring(() => ({
      value: sensor.threshold
    }))

    // update threshold whenever it updates on state
    React.useEffect(() => {
      if (!currentlyDownRef.current) {
        setThresholdValue({ value: sensor.threshold, immediate: true })
      }
    }, [sensor.threshold, setThresholdValue])

    const thumbEnabledSpring = useSpring({
      opacity: enableThresholdChange ? 1 : 0,
      config: springConfig.stiff
    })

    const handleSensorThresholdUpdate = React.useCallback(
      (value: number) => {
        serverContext.updateSensorThreshold(
          serverAddress,
          device.id,
          sensor.sensorIndex,
          value
        )
      },
      [device.id, sensor.sensorIndex, serverAddress, serverContext]
    )

    const [throttledSensorUpdate, cancelThrottledUpdate] = useDebouncedCallback(
      handleSensorThresholdUpdate,
      200,
      { maxWait: 500 }
    )

    const bindThumb = useDrag(({ down, xy }) => {
      if (!containerRef.current || !enableThresholdChange) {
        return
      }

      const boundingRect = containerRef.current.getBoundingClientRect()
      const newValue = clamp(
        (boundingRect.height + boundingRect.top - xy[1]) / boundingRect.height,
        0,
        1
      )

      if (down) {
        setThresholdValue({ value: newValue, immediate: true })
        throttledSensorUpdate(newValue)
        currentlyDownRef.current = true
      } else {
        cancelThrottledUpdate()
        setThresholdValue({ value: newValue, immediate: true })
        handleSensorThresholdUpdate(newValue)
        currentlyDownRef.current = false
      }
    })

    return (
      <Container ref={containerRef}>
        <ThresholdBar
          style={{
            transform: thresholdValue.interpolate(value => `scaleY(${value})`)
          }}
        />

        <Bar
          style={{
            transform: sensorValue.interpolate(value => `scaleY(${value})`)
          }}
        />

        <OverThresholdBar
          style={{
            transform: interpolate(
              [sensorValue, thresholdValue],
              (sensorValue, thresholdValue) => {
                const translateY = toPercentage(-thresholdValue)
                const scaleY = Math.max(sensorValue - thresholdValue, 0)
                return `translateY(${translateY}) scaleY(${scaleY})`
              }
            )
          }}
        />

        <Thumb
          {...bindThumb()}
          style={{
            bottom: thresholdValue.interpolate(toPercentage),
            opacity: thumbEnabledSpring.opacity
          }}
        >
          {thresholdValue.interpolate(threshold =>
            (threshold * 100).toFixed(1)
          )}
        </Thumb>
      </Container>
    )
  }
)

export default Sensor
