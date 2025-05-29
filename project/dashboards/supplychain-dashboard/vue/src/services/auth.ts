import http from '@/utils/http'
import type { AxiosResponse } from 'axios'
import axios from 'axios'
import router from '@/router'

/** shape of the token endpoint’s JSON */
interface TokenResponse {
  access: string
  refresh: string
}

/**
 * Log in with username & password.
 * Saves access & refresh tokens to localStorage.
 */
export async function login(username: string, password: string): Promise<void> {
  const resp: AxiosResponse<TokenResponse> = await http.post('/api/token/', {
    username,
    password,
  });
  localStorage.setItem('access', resp.data.access);
  localStorage.setItem('refresh', resp.data.refresh);
}

/** Remove tokens from storage */
export function logout(): void {
  localStorage.removeItem('access');
  localStorage.removeItem('refresh');

  router.push({ 
    name: 'login', 
  });
}

/**
 * Manually refresh the access token.
 */
export async function refreshToken() {
  const refresh = localStorage.getItem('refresh')

  if (!refresh) throw new Error("No refresh token")

  const resp: AxiosResponse<{ access: string }> = await axios.post('/api/token/refresh/', { refresh })

  localStorage.setItem('access', resp.data.access)

  http.defaults.headers.common['Authorization'] = `Bearer ${resp.data.access}`
}


export function isAuthenticated(): Boolean {
  return !!localStorage.getItem('access');
}
