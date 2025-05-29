
import { defineStore } from 'pinia'
import http from '@/utils/http'

/**
 * The shape of a “nested” product as returned inside components[]
 */
export interface NestedProduct {
  id: number
  product_key: string
  product_type: number | null
  batch: string
  owner: number | null
  created_timestamp: string
}

/** Now each component entry has the full NestedProduct plus a quantity */
export interface ProductComponent {
  component: NestedProduct
  quantity: number
}

export interface Product {
  id: number
  product_key: string
  product_type: number | null
  batch: string
  owner: number | null
  owner_name: string | null
  created_timestamp: string
  components: ProductComponent[]
  recorded_by: number | null
}

/** Interface matching DRF ProductEventSerializer */
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

interface ProductState {
  // Single-product detail
  product: Product | null
  loading: boolean
  error: string | null

  // List of products for an order
  productsList: Product[]
  listLoading: boolean
  listError: string | null

  // List of product events
  events: ProductEvent[],
  eventsLoading: boolean,
  eventsError: string | null

  // JWT token for pagination
  jwt: string | null
}

export const useProductStore = defineStore('product', {
  state: (): ProductState => ({
    product: null,
    loading: false,
    error: null,

    productsList: [],
    listLoading: false,
    listError: null,

    events: [],
    eventsLoading: false,
    eventsError: null,

    jwt: null,
  }),

  actions: {
    /** Fetch a single product by ID */
    async fetchProduct(id: string | number) {
      this.loading = true
      this.error = null

      try {
        const params: Record<string,string> = {}
        if (this.jwt) params.token = String(this.jwt);

        const { data } = await http.get<Product>(`/api/products/${id}/`, { params })

        this.product = data
      } catch (err: any) {
        this.error = err.message || String(err)
      } finally {
        this.loading = false
      }
    },

    /**
     * Create a new product
     * @returns created Product
     */
    async createProduct(payload: Partial<Product>) {
      this.loading = true
      this.error = null
      try {
        const { data } = await http.post<Product>('/api/products/', payload)
        this.productsList.unshift(data)
        return data
      } catch (err: any) {
        this.error = err.response?.data?.detail || err.message || String(err)
        throw err
      } finally {
        this.loading = false
      }
    },

    /** Fetch all products linked to this company */
    async fetchProducts() {
      this.listLoading = true
      this.listError = null

      try {
        const { data } = await http.get<Product[]>(
          `/api/products/`,
        )
        this.productsList = data
      } catch (err: any) {
        this.listError = err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Fetch all products linked to a given ProductOrder */
    async fetchProductsByOrder(orderId: string | number) {
      this.listLoading = true
      this.listError = null

      try {
        const params: Record<string,string> = {}
        if (this.jwt) params.token = String(this.jwt);

        const { data } = await http.get<Product[]>(
          `/api/products/order/${orderId}/`, { params }
        )
        this.productsList = data
      } catch (err: any) {
        this.listError = err.message || String(err)
      } finally {
        this.listLoading = false
      }
    },

    /** Fetch all product events */
    async fetchProductEvents(id: string | number) {
      this.eventsLoading = true
      this.eventsError = null

      try {
        const params: Record<string,string> = {}
        if (this.jwt) params.token = String(this.jwt);

        const { data } = await http.get<ProductEvent[]>(
          `/api/products/${id}/events/`, { params }
        )
        this.events = data
      } catch (err: any) {
        this.eventsError = err.message || String(err)
      } finally {
        this.eventsLoading = false
      }
    },

    /** Clear single-product state */
    clearProduct() {
      this.product = null
      this.error = null
    },

    /** Clear products list state */
    clearProductsList() {
      this.productsList = []
      this.listError = null
    },

    /** Clear product events state */
    clearProductEvents() {
      this.events = []
      this.eventsError = null
    },

    setJwt(jwt: string | null) {
      this.jwt = jwt
    }
  },
})

