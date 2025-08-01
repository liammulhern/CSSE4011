// stores/trackerEvent.ts
import { defineStore } from 'pinia'
import http from '@/utils/http'

/** Interface matching your DRF TrackerEventSerializer */
export interface TrackerEvent {
  message_id: string
  event_type: string
  payload: Record<string, any>
  timestamp: string
  data_hash: string
  block_id: string
  created_timestamp: string
}

/** Store state */
interface TrackerEventState {
  events: TrackerEvent[]
  event: TrackerEvent | null
  loading: boolean
  error: string | null
}

export const useTrackerEventStore = defineStore('trackerEvent', {
  state: (): TrackerEventState => ({
    events: [],
    event: null,
    loading: false,
    error: null,
  }),

  actions: {
    /** Load all tracker events */
    async fetchEvents() {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<TrackerEvent[]>('/api/trackerevents/')
        this.events = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },

    /** Load a single tracker event by its message_id */
    async fetchEvent(messageId: string) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<TrackerEvent>(
          `/api/trackerevents/${messageId}/`
        )
        this.event = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },

    async fetchEventsByTracker(trackerId: number) {
      this.loading = true
      this.error = null
      try {
        const { data } = await http.get<TrackerEvent[]>(
          `/api/trackerevents/?tracker=${trackerId}`
        )
        this.events = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },

    /** Clear the single-event state */
    clearEvent() {
      this.event = null
      this.error = null
    },

    /** Clear the list-of-events state */
    clearEvents() {
      this.events = []
      this.error = null
    },
  },
})

