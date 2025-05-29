import { defineStore } from 'pinia'
import http from '@/utils/http'

/** Interface matching DRF ProductEventSerializer */
export interface ProductEvent {
  message_id: string
  product: number
  trackerevent: string | null
  event_type: string
  payload: Record<string, any>
  timestamp: string
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
        const { data } = await http.get<ProductEvent[]>('/api/productevents/')
        this.events = data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /** Load a single product event by its event_id*/
    async fetchEvent(eventId: string) {
      this.loading = true
      this.error = null

      try {
        const { data } = await http.get<ProductEvent>(
          `/api/productevents/${eventId}/`
        )
        this.event = data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /** Load product events for a specific product */
    async fetchEventsByProduct(productId: string | number) {
      this.loading = true
      this.error = null

      try {
        const resp = await http.get<ProductEvent[]>(
          `/api/productevents/?product=${productId}&ordering=-timestamp&page_size=1000`
        )
        this.events = resp.data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /** Load product events for a specific productorder */
    async fetchEventsByProductOrder(productorderId: string | number) {
      this.loading = true
      this.error = null

      try {
        const resp = await http.get<ProductEvent[]>(
          `/api/productevents/?productorder=${productorderId}&ordering=-timestamp&page_size=1000`
        )
        this.events = resp.data
      } catch (err: any) {
        this.error = err.message || String(err)
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

