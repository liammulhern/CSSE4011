
import { defineStore } from 'pinia'
import http from '@/utils/http'

/**
 * TypeScript interface matching the ProductNotificationSerializer in DRF
 */
export interface ProductNotification {
  id: number
  notication_type: 'alert' | 'notification'
  productevent: number
  requirement: string | null
  order: number | null
  timestamp: string            // read-only, server-generated
  created_timestamp: string    // read-only, server-generated
  message: string
  acknowledged_timestamp: string | null // read-only, server-generated when ack
  acknowledged_by: number | null        // read-only, server-generated when ack
}

interface NotificationState {
  /** List of all notifications */
  list: ProductNotification[]
  listLoading: boolean
  listError: string | null

  /** List of alert-type notifications */
  alerts: ProductNotification[]
  alertsLoading: boolean
  alertsError: string | null

  /** Single notification detail */
  detail: ProductNotification | null
  detailLoading: boolean
  detailError: string | null
}

export const useProductNotificationStore = defineStore('productNotification', {
  state: (): NotificationState => ({
    list: [],
    listLoading: false,
    listError: null,

    alerts: [],
    alertsLoading: false,
    alertsError: null,

    detail: null,
    detailLoading: false,
    detailError: null,
  }),

  actions: {
    /** Fetch all notifications */
    async fetchNotifications() {
      this.listLoading = true
      this.listError = null
      try {
        const { data } = await http.get<ProductNotification[]>('/api/notifications/productevent/')
        this.list = data
      } catch (err: any) {
        this.listError = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Fetch only alert-type notifications */
    async fetchAlerts() {
      this.alertsLoading = true
      this.alertsError = null
      try {
        const { data } = await http.get<ProductNotification[]>('/api/notifications/productevent/alerts/')
        this.alerts = data
      } catch (err: any) {
        this.alertsError = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.alertsLoading = false
      }
    },

    /** Fetch single notification by ID */
    async fetchNotification(id: number | string) {
      this.detailLoading = true
      this.detailError = null
      try {
        const { data } = await http.get<ProductNotification>(`/api/notifications/productevent/${id}/`)
        this.detail = data
      } catch (err: any) {
        this.detailError = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.detailLoading = false
      }
    },

    /** Create a new notification */
    async createNotification(
      payload: Omit<
        ProductNotification,
        'id' | 'timestamp' | 'created_timestamp' | 'acknowledged_timestamp' | 'acknowledged_by'
      >
    ) {
      try {
        const { data } = await http.post<ProductNotification>('/api/notifications/productevent/', payload)
        this.list.unshift(data)
        return data
      } catch (err: any) {
        throw new Error(err.response?.data?.detail || err.message || String(err))
      }
    },

    /** Acknowledge a notification */
    async acknowledgeNotification(id: number | string) {
      try {
        const { data } = await http.post<ProductNotification>(
          `/api/notifications/productevent/${id}/acknowledge/`
        )
        // update detail and lists
        this.detail = data
        const idx = this.list.findIndex((n) => n.id === data.id)
        if (idx !== -1) this.list.splice(idx, 1, data)
        const alertIdx = this.alerts.findIndex((n) => n.id === data.id)
        if (alertIdx !== -1) this.alerts.splice(alertIdx, 1, data)
        return data
      } catch (err: any) {
        throw new Error(err.response?.data?.detail || err.message || String(err))
      }
    },
  },
})

