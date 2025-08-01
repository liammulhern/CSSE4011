import { defineStore } from 'pinia'
import http from '@/utils/http'
import { type ProductNotification } from './productnotification';

export const useStore = defineStore('notifications', {
  state: () => ({
    listLoading: false,
    listError: null,
    notifications: [],
    timeWindow: { start: null as Date | null, end: null as Date | null }
  }),
  getters: {
    notificationsInWindow(state) {
      if (!state.timeWindow.start || !state.timeWindow.end) return state.notifications
      return state.notifications.filter(n => {
        const t = new Date(n.timestamp)
        return t >= state.timeWindow.start && t <= state.timeWindow.end
      })
    }
  },
  actions: {
    async fetchNotifications() {
      this.listLoading = true
      this.listError = null
      try {
        const { data } = await http.get<ProductNotification[]>('/api/notifications/productevent/')
        this.notifications = data
      } catch (err: any) {
        this.listError = err.response?.data?.detail || err.message || String(err)
      } finally {
        this.listLoading = false
      }
    }
  }
})

