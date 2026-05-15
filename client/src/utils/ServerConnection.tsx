import {
  DeviceConfiguration,
  DeviceInputData,
  DeviceDescriptionMap
} from '../../../common-types/device'

import SubscriptionManager from './SubscriptionManager'

interface ServerConnectionSettings {
  address: string
  onConnect: () => void
  onDisconnect: () => void
  onDevicesUpdated: (devices: DeviceDescriptionMap) => void
}

class ServerConnection {
  private ws: WebSocket | null = null
  private settings: ServerConnectionSettings
  private inputEventSubscriptions: SubscriptionManager<DeviceInputData>
  private rateEventSubscriptions: SubscriptionManager<number>
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null
  private reconnectDelay = 250
  private subscribedDevices: Set<string> = new Set()

  constructor(settings: ServerConnectionSettings) {
    this.settings = settings
    this.inputEventSubscriptions = new SubscriptionManager()
    this.rateEventSubscriptions = new SubscriptionManager()
    this.connect()
  }

  private getWsUrl = (): string => {
    const address = this.settings.address
    if (address.startsWith('ws://') || address.startsWith('wss://')) {
      return address
    }
    return `ws://${address.replace(/^https?:\/\//, '')}`
  }

  private connect = () => {
    try {
      this.ws = new WebSocket(this.getWsUrl())
    } catch {
      this.scheduleReconnect()
      return
    }

    this.ws.onopen = () => {
      this.reconnectDelay = 250
      this.settings.onConnect()
      for (const deviceId of this.subscribedDevices) {
        this.send('subscribeToDevice', { deviceId })
      }
    }

    this.ws.onclose = () => {
      this.settings.onDisconnect()
      this.scheduleReconnect()
    }

    this.ws.onerror = () => {
      // onclose fires after onerror; reconnect is handled there
    }

    this.ws.onmessage = (event: MessageEvent) => {
      this.handleMessage(event.data)
    }
  }

  private scheduleReconnect = () => {
    if (this.reconnectTimer !== null) return
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null
      this.connect()
    }, this.reconnectDelay)
    this.reconnectDelay = Math.min(this.reconnectDelay * 2, 1000)
  }

  private send = (action: string, data: unknown) => {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ action, data }))
    }
  }

  private handleMessage = (raw: string) => {
    try {
      const { action, data } = JSON.parse(raw)
      switch (action) {
        case 'devicesUpdated':
          this.settings.onDevicesUpdated(data.devices)
          break
        case 'inputEvent':
          this.inputEventSubscriptions.emit(data.deviceId, data.inputData)
          break
        case 'eventRate':
          this.rateEventSubscriptions.emit(data.deviceId, data.eventRate)
          break
      }
    } catch (e) {
      console.error('ServerConnection :: message parse error', e)
    }
  }

  private subscribeToDevice = (deviceId: string) => {
    this.subscribedDevices.add(deviceId)
    this.send('subscribeToDevice', { deviceId })
  }

  private unsubscribeFromDevice = (deviceId: string) => {
    this.subscribedDevices.delete(deviceId)
    this.send('unsubscribeFromDevice', { deviceId })
  }

  private hasAnySubscriptionsForDevice = (deviceId: string) => {
    return (
      this.inputEventSubscriptions.hasSubscriptionsFor(deviceId) ||
      this.rateEventSubscriptions.hasSubscriptionsFor(deviceId)
    )
  }

  public subscribeToInputEvents = (
    deviceId: string,
    callback: (data: DeviceInputData) => void
  ) => {
    if (!this.hasAnySubscriptionsForDevice(deviceId)) {
      this.subscribeToDevice(deviceId)
    }
    this.inputEventSubscriptions.subscribe(deviceId, callback)

    return () => {
      this.inputEventSubscriptions.unsubscribe(deviceId, callback)
      if (!this.hasAnySubscriptionsForDevice(deviceId)) {
        this.unsubscribeFromDevice(deviceId)
      }
    }
  }

  public subscribeToRateEvents = (
    deviceId: string,
    callback: (rate: number) => void
  ) => {
    if (!this.hasAnySubscriptionsForDevice(deviceId)) {
      this.subscribeToDevice(deviceId)
    }
    this.rateEventSubscriptions.subscribe(deviceId, callback)

    return () => {
      this.rateEventSubscriptions.unsubscribe(deviceId, callback)
      if (!this.hasAnySubscriptionsForDevice(deviceId)) {
        this.unsubscribeFromDevice(deviceId)
      }
    }
  }

  public updateConfiguration = (
    deviceId: string,
    configuration: Partial<DeviceConfiguration>,
    store: boolean
  ) => {
    this.send('updateConfiguration', { deviceId, configuration, store })
  }

  public updateSensorThreshold = (
    deviceId: string,
    sensorIndex: number,
    newThreshold: number,
    store: boolean
  ) => {
    this.send('updateSensorThreshold', { deviceId, sensorIndex, newThreshold, store })
  }

  public calibrate = (deviceId: string, calibrationBuffer: number) => {
    this.send('calibrate', { deviceId, calibrationBuffer })
  }
}

export default ServerConnection
