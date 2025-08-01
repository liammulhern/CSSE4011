// stores/tracker.ts
import { defineStore } from 'pinia'
import http from '@/utils/http'

export interface Tracker {
  id: number
  tracker_key: string
  // add any other fields you need
}

export const useTrackerStore = defineStore('tracker', {
  state: () => ({
    trackers: [] as Tracker[],
    loading: false,
    error: null as string | null,
  }),
  actions: {
    async fetchTrackers() {
      this.loading = true
      this.error = null
      try {
        const { data } = await http.get<Tracker[]>('/api/trackers/')
        this.trackers = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },
  },
})

