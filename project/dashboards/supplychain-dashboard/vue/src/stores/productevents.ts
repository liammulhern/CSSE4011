import { defineStore } from 'pinia'
import http from '@/utils/http'

/** Interface matching your DRF ProductEventSerializer */
export interface ProductEvent {
  message_id: string
  product: number
  trackerevent: string | null
  event_type: string
  payload: Record<string, any>
  timestamp: string
  data_hash: string
  data_uri: string
  recorded_by: number | null
  created_timestamp: string
}

/** Store state */
interface ProductEventState {
  events: ProductEvent[]
  event: ProductEvent | null
  loading: boolean
  error: string | null
}

export const useProductEventStore = defineStore('productEvent', {
  state: (): ProductEventState => ({
    events: [],
    event: null,
    loading: false,
    error: null,
  }),

  actions: {
    /** Load all product events */
    async fetchEvents() {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<ProductEvent[]>('/api/product-events/')
        this.events = data
      } catch (err) {
        this.error = err instanceof Error ? err.message : String(err)
      } finally {
        this.loading = false
      }
    },

    /** Load a single product event by its message_id */
    async fetchEvent(messageId: string) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<ProductEvent>(
          `/api/product-events/${messageId}/`
        )
        this.event = data
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
